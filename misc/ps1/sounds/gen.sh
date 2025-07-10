rm "audio.zip" *.wav
wget "classicube.net/static/audio.zip"
unzip "audio.zip"

for i in *.wav; do
    ~/repos/psxavenc/build/psxavenc -t spu -f 32000 $i "${i%.wav}.snd"
done

rm *.wav
