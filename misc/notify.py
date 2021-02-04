import requests, re, datetime, json, time, os
# Any resemblence to any discord bots written by 123DMWM is purely coincidental. I swear.

# The URL of the discord channel webhook to notify
# You can generate a webhook by right clicking channel -> Edit Channel -> Webhooks
WEBHOOK_URL = ''
# The ID of the user you want to ping on a build failure
# You can get this by right clicking user -> Copy ID
# You may have to go to settings -> Appearance -> Check Developer Mode to see this option
TARGET_USER = ''

def notify_webhook(body):
	try:
		webhook_data = {
			"username": "CC BuildBot",
			"avatar_url": "https://static.classicube.net/img/cc-cube-small.png",
			"content" : body
		}
		r = requests.post(WEBHOOK_URL, json=webhook_data)
		print("Webhook response: " + r.text)
	except Exception as e:
		print("Webhook failed: %s" % (e))

def build_exists(file):
	return os.path.exists('client/src/' + file)

builds = {
	'Windows32' : build_exists('cc-w32-d3d.exe'),
	'Windows64' : build_exists('cc-w64-d3d.exe'),
	'macOS32'   : build_exists('cc-osx32'),
	'macOS64'   : build_exists('cc-osx64'),
	'Linux32'   : build_exists('cc-nix32'),
	'Linux64'   : build_exists('cc-nix64'),
	'RPi'       : build_exists('cc-rpi'),
	'Web'       : build_exists('cc.js'),
	'Android'   : build_exists('cc.apk'),
	'Win-ogl32' : build_exists('cc-w32-ogl.exe'),
	'Win-ogl64' : build_exists('cc-w64-ogl.exe'),
}

failed = []

def check_build(key):
	key_32 = key + '32'
	key_64 = key + '64'

	if key in builds:
		if not builds[key]:
			failed.append(key)
	else:
		if not builds[key_32] and not builds[key_64]:
			failed.append(key + ' 32/64')
		elif not builds[key_32]:
			failed.append(key + ' 32')
		elif not builds[key_64]:
			failed.append(key + ' 64')

check_build('Windows')
check_build('macOS')
check_build('Linux')
check_build('RPi')
check_build('Web')
check_build('Android')
check_build('Win-ogl')

if len(failed):
	notify_webhook('<@%s>, failed to compile for: %s' % (TARGET_USER, ', '.join(failed)))
