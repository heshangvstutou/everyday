#!/usr/bin/python
#coding=utf-8

##删除当前目录和子目录下的所有pyc文件

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
					cmd = 'rm '+root+'/'+fileName
				if cmd:
					os.system(cmd)
		else:
			firstFlag = 1
print 'Done!'

#/data/192.168.8.201/jlink/jlink_mt6572kk/tools
