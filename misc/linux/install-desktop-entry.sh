DESKTOP_FILE=ClassiCube.desktop
GAME_DIR=`pwd`

# remove existing ClassiCube desktop entry file
rm $DESKTOP_FILE

# download ClassiCube icon from github if necessary
if [ -f "CCicon.png" ]
then
  echo "CCicon.png exists already. Skipping download."
else
  echo "CCicon.png doesn't exist. Attempting to download it.."
  wget "https://raw.githubusercontent.com/UnknownShadow200/classicube/master/misc/CCicon.png"
fi

# create ClassiCube desktop entry
echo 'Creating ClassiCube.desktop..'
cat >> $DESKTOP_FILE << EOF
[Desktop Entry]
Type=Application
Comment=Minecraft Classic inspired sandbox game
Name=ClassiCube
Exec=$GAME_DIR/ClassiCube
Icon=$GAME_DIR/CCicon.png
Path=$GAME_DIR
Terminal=false
Categories=Game
EOF
chmod +x $DESKTOP_FILE

echo 'Installing ClassiCube.desktop..'
# install ClassiCube desktop entry into the system
sudo desktop-file-install --dir=/usr/share/applications ClassiCube.desktop
sudo update-desktop-database /usr/share/applications
