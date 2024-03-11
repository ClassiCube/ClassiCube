# upload_build.sh [TARGET NAME] [SOURCE FILE]
# e.g. ~/upload_build.sh latest/ClassiCube.ipa cc.ipa
API_URL=
API_KEY=

if [ -s $2 ]; then
  curl $API_URL \
   --header "X-Build-Key:$API_KEY" \
   --header "X-Build-Name:$1" \
   --data-binary @$2
else
  echo "Missing or empty file: $2"
fi
