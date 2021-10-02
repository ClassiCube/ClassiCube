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
		print("BuildNotify response: " + r.text)
	except Exception as e:
		print("BuildNotify failed: %s" % (e))

cc_errors = None
try:
	with open('client/cc_errors.txt', 'r') as file:
		cc_errors = file.read()
except FileNotFoundError:
	# nothing to as no compile errors
	pass

if cc_errors:
	notify_webhook('**Houston, we have a problem** <@%s>\n\n%s' % (TARGET_USER, cc_errors))