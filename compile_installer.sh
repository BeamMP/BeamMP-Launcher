#!/bin/bash
if [ ! -f "./bin/BeamMP-Launcher" ]; then
    echo "Please build BeamMP first"
    exit 1
fi

echo "Setting up"
cd data
cp installer.sh ./BeamMP_Installer.sh

# Check for libraries automagically
if command -v ldd > /dev/null; then
    echo "Adding checks for libraries"
    LIBS="$(ldd ../bin/BeamMP-Launcher | awk '{print $1}' | grep -v "linux-vdso.so" | grep -v "ld-linux")"
    LIBREQUEST="LIBREQUEST=\"\""
    while IFS= read -r LIB; do
        LIBREQUEST="$LIBREQUEST\nif [ ! -n \"\$(ldconfig -p | grep \"$LIB\")\" ]; then\nLIBREQUEST=\"$LIB \$LIBREQUEST\"\nfi"
    done <<< "$LIBS"
    sed -i "s/SETUPLIBREQUEST/$LIBREQUEST/g" ./BeamMP_Installer.sh
else
    echo "Skipping library checks due to lack of ldd (What system are you running???)"
    sed -i "s/SETUPLIBREQUEST/LIBREQUEST=\"\"/g" ./BeamMP_Installer.sh
fi

# Languages
echo "Generating locales"
NL=$'\n'
LANGS=""
LANGSEXEC="if [[ \"\$LANG\" == \"1\" ]]; then"
NUMLANG=1
for FILE in ./locales/*; do
    DAT="$(cat "$FILE" | sed 's/^[[:space:]]*//g' | sed '/^[[:blank:]]*#/d;s/#.*//' | sed '/^[[:blank:]]*$/d')"
    OUT="$(echo "$DAT" | tail -n +2)"
    LANGNAME="$(echo "$DAT" | head -n +1)"
    LANGS="$LANGS \"$NUMLANG\" \"$LANGNAME\""
    if ((NUMLANG > 1)); then
        LANGSEXEC="$LANGSEXEC${NL}elif [[ \"\$LANG\" == \"$NUMLANG\" ]]; then"
    fi
    LANGSEXEC="$LANGSEXEC${NL}$OUT"
    NUMLANG=$((NUMLANG+1))
done
LANGSEXEC="$(echo "$LANGSEXEC${NL}else${NL}echo \"0x1 Unsupported Language\"${NL}exit 1${NL}fi" | sed "s/\\\\/\\\\\\\\/g" | sed "s/\\//\\\\\\//g" | sed "s/\\\$LANG/\\\\\\\$LANG/g")"

perl -i -pe "s/SETUPLANGS0/$LANGS/" ./BeamMP_Installer.sh
perl -i -pe "s/SETUPLANGS1/$LANGSEXEC/" ./BeamMP_Installer.sh

# Minify
echo "Cleaning up file"
sed -i '/^[[:blank:]]*#/d;s/#.*//' ./BeamMP_Installer.sh # Remove comments
sed -i 's/^[[:space:]]*//g' ./BeamMP_Installer.sh # Remove whitespace
sed -i '/^[[:blank:]]*$/d' ./BeamMP_Installer.sh # Remove empty lines (ALWAYS USE \n!!)

# Add binaries
echo "Adding and linking files to script"
echo -e "\nexit" >> ./BeamMP_Installer.sh
LINES0="$(wc -l < ./BeamMP_Installer.sh)"
LINES1="$(wc -l < ../bin/BeamMP-Launcher)"
sed -i "s/+FILELINELENGTH0/+$((LINES0+1))/g" ./BeamMP_Installer.sh
sed -i "s/OUTFILELINELENGTH0/$((LINES1+1))/g" ./BeamMP_Installer.sh
cat ../bin/BeamMP-Launcher >> ./BeamMP_Installer.sh
echo >> ./BeamMP_Installer.sh
LINES0="$(wc -l < ./BeamMP_Installer.sh)"
LINES1="$(wc -l < ./BeamMP.ico)"
sed -i "s/+FILELINELENGTH1/+$((LINES0+1))/g" ./BeamMP_Installer.sh
sed -i "s/OUTFILELINELENGTH1/$((LINES1+1))/g" ./BeamMP_Installer.sh
cat ./BeamMP.ico >> ./BeamMP_Installer.sh
echo >> ./BeamMP_Installer.sh
LINES0="$(wc -l < ./BeamMP_Installer.sh)"
LINES1="$(wc -l < ./BeamMP.desktop)"
sed -i "s/+FILELINELENGTH2/+$((LINES0+1))/g" ./BeamMP_Installer.sh
sed -i "s/OUTFILELINELENGTH2/$((LINES1+1))/g" ./BeamMP_Installer.sh
cat ./BeamMP.desktop >> ./BeamMP_Installer.sh

# Finish up
echo "Finishing up"
sed -i '1s/^/#!\/usr\/bin\/env bash\n/' ./BeamMP_Installer.sh
chmod u+x ./BeamMP_Installer.sh
mkdir ../bin &>/dev/null
mv ./BeamMP_Installer.sh ../bin
