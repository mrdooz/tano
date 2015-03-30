import os
import time
import glob
import subprocess
import collections
import platform
import pickle

SETTINGS = { 
	# Darwin is broken for now
	'Darwin': { 'dir': '/Users/dooz/OneDrive/gfx', 'conv': None },
	'Windows': { 
		'input_dir': 'd:/onedrive/tano/c4d', 
		'output_dir': 'd:/projects/tano/gfx',
		'conv': 'D:/projects/melange_exporter/_win32/Release/exporter.exe'
	}
}

system = platform.system()
settings = SETTINGS.get(system, None)
if dir is None:
	print 'Unknown system: %s' % system
	exit(1)

input_dir, output_dir = settings.get('input_dir', None), settings.get('output_dir', None)
if input_dir is None or output_dir is None:
	print 'Unable to find directory; input: %s, output: %s' % (input_dir, output_dir)
	exit(1)
input_dir = os.path.normpath(input_dir)
output_dir = os.path.normpath(output_dir)

conv = settings.get('conv', None)
if conv is None:
	print 'Unable to find converter'
	exit(1)

while True:
	for c4d_name in glob.glob(os.path.join(input_dir, '*.c4d')):
		# check if the converted mesh is older than the c4d file
		c4d_time = os.path.getmtime(c4d_name)
		path, filename = os.path.split(c4d_name)
		head, tail = os.path.splitext(filename)
		mesh_name = os.path.join(output_dir, head) + '.boba'
		should_convert = False
		try:
			mesh_time =  os.path.getmtime(mesh_name)
			should_convert = c4d_time > mesh_time
		except:
			should_convert = True

		if should_convert:
			print '[%s] Converting: %s -> %s' % (time.ctime(), c4d_name, mesh_name)
			res = subprocess.call([conv, c4d_name, mesh_name])
	time.sleep(1)
