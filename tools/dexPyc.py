#!/usr/bin/python
#coding=utf-8

##反编译当前目录下的所以pyc文件为py源码，使用的工具为uncompyle2，需要单独下载安装
###并且本地python版本需要为2.7以上

import os
import py_compile

#codePath='/data/jlink_sw/jlinktools/product/'
#20150811codePath = os.getcwd()+'/jlinkCustomTools'
codePath = os.getcwd()
firstFlag = 0
for root,dirs,files in os.walk(codePath, True):
	#print root
	#print dirs
	#print files
	if root.find('.git') != -1 or root.find('.repo') != -1: # or root.find('/lib/androidTools') != -1:
		dirs[:] = [] # 忽略当前目录下的子目录
	else:
		if firstFlag != 0:
			#cmd = 'rm '+root+'/*.pyc'
			#os.system(cmd)
			for fileName in files:
				cmd = ''
				if fileName.find('~') != -1:
					cmd = 'rm '+root+'/'+fileName
				elif os.path.splitext(fileName)[1] == '.pyc':
					#cmd = 'ls  '+root+'/'+fileName[0]
					outFileName=os.path.splitext(fileName)[0]+'.py'
					outPathFileName=root+'/'+outFileName
					print outFileName  ,fileName
					cmd ='uncompyle2 '+root+'/'+fileName+' > '+outPathFileName
					print cmd
				if cmd:
					os.system(cmd)
		else:
			firstFlag = 1
print 'Done!'

#/data/192.168.8.201/jlink/jlink_mt6572kk/tools
