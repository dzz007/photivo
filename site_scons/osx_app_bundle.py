#! /usr/bin/env python

import os
from os.path import exists
import shutil

#name of photivo directory
photivo_dir='photivo-s'
home_dir = os.getenv("HOME")
source_dir=home_dir +'/'+photivo_dir
target_dir =  home_dir +'/'+photivo_dir + "/Photivo.app/Contents"

AppBundle= home_dir + '/' + photivo_dir + "/Photivo.app"

os.makedirs(target_dir)
shutil.copy(source_dir+'/Info.plist', target_dir)
os.chdir( target_dir )

dir_tree = 'Frameworks/MacOS/Resources'
tree_group = dir_tree.split("/")

for item in tree_group: # Removes any empty strings from the list
  
  if item == "":
      tree_group.remove(item)
  #os.mkdir(item)
  if item=='MacOS':
  	os.mkdir(item)
  	for elm in ['Curves','LensfunDatabase','ChannelMixers','Presets','Profiles','Themes','photivo']:
  		if os.path.isdir(source_dir + '/' + elm) == True:
  			print elm + ' dir copied'
  			shutil.copytree(source_dir + '/' + elm, item + '/' + elm)
  			#distutils.dir_util.copy_tree
  		else:
  			print elm + ' file copied'
  			shutil.copy(source_dir + '/' + elm, item)
  elif item=='Resources':
  	os.mkdir(item)
  	shutil.copy(source_dir+'/photivo-appicon.icns', item)
  else:
  	os.mkdir(item)
  	
  	

#print AppBundle
from subprocess import call

if call('/usr/bin/macdeployqt ' + AppBundle , shell=True)==0:
	print 'Bundle building finished!'
else:
	print 'There was a problem building the bundle'
