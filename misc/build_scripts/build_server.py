from http.server import HTTPServer, SimpleHTTPRequestHandler
import os
import subprocess

build_files = { 
	'/ClassiCube.exe'    : 'cc-w32-d3d.exe', '/ClassiCube.opengl.exe'    : 'cc-w32-ogl.exe', 
	'/ClassiCube.64.exe' : 'cc-w64-d3d.exe', '/ClassiCube.64-opengl.exe' : 'cc-w64-ogl.exe', 
	'/ClassiCube.32'     : 'cc-nix32',       '/ClassiCube'               : 'cc-nix64', 
	'/ClassiCube.osx'    : 'cc-osx32',       '/ClassiCube.64.osx'        : 'cc-osx64',
	'/ClassiCube.js'     : 'cc.js',          '/ClassiCube.apk'           : 'cc.apk',
	'/ClassiCube.rpi'    : 'cc-rpi',
	'/cc-nix32-gl2'      : 'cc-nix32-gl2',   '/cc-nix64-gl2'             : 'cc-nix64-gl2',
	'/cc-osx32-gl2'      : 'cc-osx32-gl2',   '/cc-osx64-gl2'             : 'cc-osx64-gl2',
}

release_files = { 
	'/win32' : 'win32/ClassiCube.exe',    '/win64' : 'win64/ClassiCube.exe',
	'/osx32' : 'osx32/ClassiCube.tar.gz', '/osx64' : 'osx64/ClassiCube.tar.gz',
	'/mac32' : 'mac32/ClassiCube.tar.gz', '/mac64' : 'mac64/ClassiCube.tar.gz',
	'/nix32' : 'nix32/ClassiCube.tar.gz', '/nix64' : 'nix64/ClassiCube.tar.gz',
	'/rpi32' : 'rpi32/ClassiCube.tar.gz',
}

def run_script(file):
	args = ["sh", file]
	su   = subprocess.Popen(args)
	return su.wait()

class Handler(SimpleHTTPRequestHandler):

	def serve_script(self, file, msg):
		ret = run_script(file)
		self.send_response(200)
		self.end_headers()
		self.wfile.write(msg % ret)

	def do_GET(self):
		if self.path in build_files:
			self.serve_exe('client/src/'     + build_files[self.path])
		elif self.path in release_files:
			self.serve_exe('client/release/' + release_files[self.path])
		elif self.path == '/rebuild':
			self.serve_script('build.sh', 'Rebuild client (%s)')
		elif self.path == '/release':
			self.serve_script('build_release.sh', 'Package release (%s)')
		else:
			self.send_error(404, "Unknown action")
		return

	def serve_exe(self, path):
		try:
			f = open(path, 'rb')
			fs = os.fstat(f.fileno())
			self.send_response(200)
			self.send_header("Content-type", "application/octet-stream")
			self.send_header("Content-Length", str(fs[6]))
			self.end_headers()
			self.copyfile(f, self.wfile)
			f.close()
		except IOError:
			self.send_error(404, "File not found")

httpd = HTTPServer(('', 80), Handler)
httpd.serve_forever()
