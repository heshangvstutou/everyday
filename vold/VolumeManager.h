/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_VOLD_VOLUME_MANAGER_H
#define ANDROID_VOLD_VOLUME_MANAGER_H

#include <pthread.h>
#include <fnmatch.h>
#include <stdlib.h>

#ifdef __cplusplus

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <cutils/multiuser.h>
#include <utils/List.h>
#include <utils/Timers.h>
#include <sysutils/SocketListener.h>
#include <sysutils/NetlinkEvent.h>

#include "Disk.h"
#include "VolumeBase.h"

/* The length of an MD5 hash when encoded into ASCII hex characters */
#define MD5_ASCII_LENGTH_PLUS_NULL ((MD5_DIGEST_LENGTH*2)+1)

typedef enum { ASEC, OBB } container_type_t;

class ContainerData {
public:
    ContainerData(char* _id, container_type_t _type)
            : id(_id)
            , type(_type)
    {}

    ~ContainerData() {
        if (id != NULL) {
            free(id);
            id = NULL;
        }
    }

    char *id;
    container_type_t type;
};

typedef android::List<ContainerData*> AsecIdCollection;

class VolumeManager {
public:
    static const char *SEC_ASECDIR_EXT;
    static const char *SEC_ASECDIR_INT;
    static const char *ASECDIR;
    static const char *LOOPDIR;

private:
    static VolumeManager *sInstance;

    SocketListener        *mBroadcaster;

    AsecIdCollection      *mActiveContainers;
    bool                   mDebug;
    bool                   mHandleUevent;

public:
    virtual ~VolumeManager();

    // TODO: pipe all requests through VM to avoid exposing this lock
    std::mutex& getLock() { return mLock; }

    int start();
    int stop();

    void handleBlockEvent(NetlinkEvent *evt);

    class DiskSource {
    public:
        DiskSource(const std::string& sysPattern, const std::string& nickname, int flags) :
                mSysPattern(sysPattern), mNickname(nickname), mFlags(flags) {
        }

        bool matches(const std::string& sysPath) {
            return !fnmatch(mSysPattern.c_str(), sysPath.c_str(), 0);
        }

        const std::string& getNickname() { return mNickname; }
        int getFlags() { return mFlags; }

    private:
        std::string mSysPattern;
        std::string mNickname;
        int mFlags;
    };

    void addDiskSource(const std::shared_ptr<DiskSource>& diskSource);

    std::shared_ptr<android::vold::Disk> findDisk(const std::string& id);
    std::shared_ptr<android::vold::VolumeBase> findVolume(const std::string& id);

    void listVolumes(android::vold::VolumeBase::Type type, std::list<std::string>& list);

    nsecs_t benchmarkPrivate(const std::string& id);

    int forgetPartition(const std::string& partGuid);

    int onUserAdded(userid_t userId, int userSerialNumber);
    int onUserRemoved(userid_t userId);
    int onUserStarted(userid_t userId);
    int onUserStopped(userid_t userId);

    int setPrimary(const std::shared_ptr<android::vold::VolumeBase>& vol);

    int remountUid(uid_t uid, const std::string& mode);

    /* Reset all internal state, typically during framework boot */
    int reset();
    /* Prepare for device shutdown, safely unmounting all devices */
    int shutdown();
    /* Unmount all volumes, usually for encryption */
    int unmountAll();

    /* ASEC */
    int findAsec(const char *id, char *asecPath = NULL, size_t asecPathLen = 0,
            const char **directory = NULL) const;
    int createAsec(const char *id, unsigned numSectors, const char *fstype,
                   const char *key, const int ownerUid, bool isExternal);
    int resizeAsec(const char *id, unsigned numSectors, const char *key);
    int finalizeAsec(const char *id);

    /**
     * Fixes ASEC permissions on a filesystem that has owners and permissions.
     * This currently means EXT4-based ASEC containers.
     *
     * There is a single file that can be marked as "private" and will not have
     * world-readable permission. The group for that file will be set to the gid
     * supplied.
     *
     * Returns 0 on success.
     */
    int fixupAsecPermissions(const char *id, gid_t gid, const char* privateFilename);
    int destroyAsec(const char *id, bool force);
    int mountAsec(const char *id, const char *key, int ownerUid, bool readOnly);
    int unmountAsec(const char *id, bool force);
    int renameAsec(const char *id1, const char *id2);
    int getAsecMountPath(const char *id, char *buffer, int maxlen);
    int getAsecFilesystemPath(const char *id, char *buffer, int maxlen);

    /* Loopback images */
    int listMountedObbs(SocketClient* cli);
    int mountObb(const char *fileName, const char *key, int ownerUid);
    int unmountObb(const char *fileName, bool force);
    int getObbMountPath(const char *id, char *buffer, int maxlen);

    /* Shared between ASEC and Loopback images */
    int unmountLoopImage(const char *containerId, const char *loopId,
            const char *fileName, const char *mountPoint, bool force);

    int setDebug(bool enable);

    void setBroadcaster(SocketListener *sl) { mBroadcaster = sl; }
    SocketListener *getBroadcaster() { return mBroadcaster; }

    static VolumeManager *Instance();

    static char *asecHash(const char *id, char *buffer, size_t len);

    /*
     * Ensure that all directories along given path exist, creating parent
     * directories as needed.  Validates that given path is absolute and that
     * it contains no relative "." or ".." paths or symlinks.  Last path segment
     * is treated as filename and ignored, unless the path ends with "/".  Also
     * ensures that path belongs to a volume managed by vold.
     */
    int mkdirs(char* path);

private:
    VolumeManager();
    void readInitialState();
    bool isMountpointMounted(const char *mp);
    bool isAsecInDirectory(const char *dir, const char *asec) const;
    bool isLegalAsecId(const char *id) const;

    int linkPrimary(userid_t userId);

    std::mutex mLock;

    std::list<std::shared_ptr<DiskSource>> mDiskSources;
    std::list<std::shared_ptr<android::vold::Disk>> mDisks;

    std::unordered_map<userid_t, int> mAddedUsers;
    std::unordered_set<userid_t> mStartedUsers;

    std::shared_ptr<android::vold::VolumeBase> mInternalEmulated;
    std::shared_ptr<android::vold::VolumeBase> mPrimary;

public:
    int mUmsSharingCount;
    int mSavedDirtyRatio;
    int mUmsDirtyRatio;

    int USBEnable(bool enable);
    void mountallVolumes();

    int shareVolume(const std::string& idl, const char *method);
    int unshareVolume(const std::string& id, const char *method);
    int shareEnabled(const std::string& id, const char *method, bool *enabled);
    int ipo(const char *cmd);

private:
    int dirtyRatio();
};

extern "C" {
#endif /* __cplusplus */
#define UNMOUNT_NOT_MOUNTED_ERR -2
    int vold_unmountAll(void);
#ifdef __cplusplus
}
#endif

#include "CFG_OMADMUSB_File.h"
#define EXT_SDCARD_TOOL "/data/ext_sdcard_tool"
#include <mntent.h>

#ifdef __cplusplus
extern "C" {
#endif
    int get_boot_mode(void);
    int is_meta_boot();
    void create_link_for_mtklogger(const char *ext_sd_path);
    char* get_usb_cmode_path();
    void get_is_nvram_in_data();
    int NvramAccessForOMADM(OMADMUSB_CFG_Struct *st, bool isRead);
    void disable_usb_by_dm();
    void invoke_coldboot();
    bool isMountpointMounted(const char *mp);
#ifdef __cplusplus
}
#endif

#endif
