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
echo "[Desktop Entry]" >> $DESKTOP_FILE
echo "Type=Application" >> $DESKTOP_FILE
echo "Comment=Minecraft Classic inspired sandbox game" >> $DESKTOP_FILE
echo "Name=ClassiCube" >> $DESKTOP_FILE
echo "Exec=$GAME_DIR/ClassiCube" >> $DESKTOP_FILE
echo "Icon=$GAME_DIR/CCicon.png" >> $DESKTOP_FILE
echo "Terminal=false" >> $DESKTOP_FILE
echo "Categories=Game;" >> $DESKTOP_FILE
chmod +x $DESKTOP_FILE

# install ClassiCube desktop entry into the system
desktop-file-install --dir=/usr/share/applications ClassiCube.desktop
update-desktop-database /usr/share/applications