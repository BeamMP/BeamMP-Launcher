#!/bin/bash

if command -v whiptail &>/dev/null; then
    WHIPTAILF=""
else
    WHIPTAILF="true"
    clear
fi

if [ -n "$WHIPTAILF" ]; then
    while true; do
        echo -e "Please enter the number associated with what language you would like use during the installation:\n\n1. English\n"
        read -p "> " LANG
        case $LANG in
            "1") break;;
            * ) clear; echo "Please provide a valid answer."; echo "";;
        esac
    done
else
    LANG=$(whiptail --title "BeamMP Installer" --nocancel --notags --menu "Select the language to use during the installation." 0 0 0 SETUPLANGS0 3>&1 1>&2 2>&3)
fi

SETUPLANGS1

if [ -n "$WHIPTAILF" ]; then
    while true; do
        clear
        echo -e "$MENU0\n"
        read -p "[$YESBUTTON/$NOBUTTON]> " RESPONSE
        case "${RESPONSE,,}" in
            "${YESBUTTON,,}" ) break;;
            "${NOBUTTON,,}" ) clear; echo "$REJECT0"; exit 0;;
            * ) clear;;
        esac
    done
else
    whiptail --title "$TITLE" --defaultno --yes-button "$ACCEPTBUTTON" --no-button "$REJECTBUTTON" --yesno "$MENU0" 0 0
    if [[ "$?" == "1" ]]; then
        echo "$REJECT0"
        exit 0
    fi
fi

STARTMENUSHORT=""
DESKTOPSHORT=""
BINSHORT=""
if [ -n "$WHIPTAILF" ]; then
    while true; do
        clear
        echo -e "$MENU1\n\n$OPTION11\n"
        read -p "[$YESBUTTON/$NOBUTTON]> " RESPONSE
        case "${RESPONSE,,}" in
            "${YESBUTTON,,}" ) clear; echo -e "$MENU1\n\n$OPTION12\n"; read -e -i "$HOME/.local/share/applications" -p "> " STARTMENUSHORT; break;;
            "${NOBUTTON,,}" ) break;;
            * ) echo;;
        esac
    done
    while true; do
        clear
        echo -e "$MENU1\n\n$OPTION21\n"
        read -p "[$YESBUTTON/$NOBUTTON]> " RESPONSE
        case "${RESPONSE,,}" in
            "${YESBUTTON,,}" ) clear; echo -e "$MENU1\n\n$OPTION22\n"; read -e -i "$HOME/Desktop" -p "> " DESKTOPSHORT; break;;
            "${NOBUTTON,,}" ) break;;
            * ) echo;;
        esac
    done
    while true; do
        clear
        echo -e "$MENU1\n\n$OPTION31\n"
        read -p "[$YESBUTTON/$NOBUTTON]> " RESPONSE
        case "${RESPONSE,,}" in
            "${YESBUTTON,,}" ) clear; echo -e "$MENU1\n\n$OPTION32\n"; read -e -i "$HOME/.local/bin" -p "> " BINSHORT; break;;
            "${NOBUTTON,,}" ) break;;
            * ) echo;;
        esac
    done
else
    eval EXTRAS=($(whiptail --nocancel --notags --title "$TITLE" --checklist "$MENU1" 0 0 0 "1" "$OPTION11" ON "2" "$OPTION21" OFF "3" "$OPTION31" ON 3>&1 1>&2 2>&3))
    for EXTRA in "${EXTRAS[@]}"; do
        if [[ "$EXTRA" == "1" ]]; then
            STARTMENUSHORT=$(whiptail --nocancel --title "$TITLE" --inputbox "$OPTION12" 0 0 "$HOME/.local/share/applications" 3>&1 1>&2 2>&3)
        fi
        if [[ "$EXTRA" == "2" ]]; then
            DESKTOPSHORT=$(whiptail --nocancel --title "$TITLE" --inputbox "$OPTION22" 0 0 "$HOME/Desktop" 3>&1 1>&2 2>&3)
        fi
        if [[ "$EXTRA" == "3" ]]; then
            BINSHORT=$(whiptail --nocancel --title "$TITLE" --inputbox "$OPTION32" 0 0 "$HOME/.local/bin" 3>&1 1>&2 2>&3)
        fi
    done
fi

if [ -n "$WHIPTAILF" ]; then
    clear
    echo -e "\n\n$OPTION41\n"
    read -e -i "$HOME/.local/share/BeamMP" -p "> " INSTALLLOC
    while true; do
        clear
        echo -e "\n\n$MENU7\n"
        read -p "[$CONTINUEBUTTON/$CANCELBUTTON]> " RESPONSE
        case "${RESPONSE,,}" in
            "${CONTINUEBUTTON,,}" ) break;;
            "${CANCELBUTTON,,}" ) break;;
            * ) echo;;
        esac
    done
else
    INSTALLLOC=$(whiptail --nocancel --title "$TITLE" --inputbox "$OPTION41" 0 0 "$HOME/.local/share/BeamMP" 3>&1 1>&2 2>&3)

    whiptail --title "$TITLE" --yes-button "$CONTINUEBUTTON" --no-button "$CANCELBUTTON" --yesno "$MENU7" 0 0
    if [[ "$?" == "1" ]]; then
        echo "$REJECT0"
        exit 0
    fi
fi

# libc.so.6, libgcc_s.so.1, libm.so.6, libstdc++.so.6, 

if command -v ldconfig > /dev/null; then
SETUPLIBREQUEST
    if [ -n "$LIBREQUEST" ]; then
        if [ -n "$WHIPTAILF" ]; then
            clear
            echo -e "$MENU41 $LIBREQUEST $MENU42\n"
            read -p "[$CONTINUEBUTTON/$CANCELBUTTON]> " RESPONSE
            case "${RESPONSE,,}" in
                "${CONTINUEBUTTON,,}" ) break;;
                "${CANCELBUTTON,,}" ) clear; echo "0x2 Potentially Unsupported"; exit 2;;
                * ) clear;;
            esac
        else
            whiptail --title "$TITLE" --defaultno --yes-button "$CONTINUEBUTTON" --no-button "$CANCELBUTTON" --yesno "$MENU41 $LIBREQUEST $MENU42" 0 0
            if [[ "$?" == "1" ]]; then
                echo "0x2 Potentially Unsupported"
                exit 2
            fi
        fi
    fi
else
    whiptail --title "$TITLE" --defaultno --yes-button "$CONTINUEBUTTON" --no-button "$CANCELBUTTON" --yesno "$MENU2" 0 0
    if [[ "$?" == "1" ]]; then
        echo "0x2 Potentially Unsupported"
        exit 2
    fi
fi

ECHOCOMMAND="true"

DOECHO() {
    if [ -n "$ECHOCOMMAND" ]; then
        echo $@
    fi
}

DOLINK() {
    ln -s $1 $2 &>/dev/null
    if [[ ! -L "$2" || ! -e "$2" ]]; then
        rm $2 &>/dev/null
        rm -f $2 &>/dev/null
        cp $1 $2 # Backup incase symlinking is broken (Dependent on your filesystem type)
    fi
}

PERFORM_SETUP() {
    DOECHO 0
    mkdir -p $INSTALLLOC &>/dev/null
    if [ ! -d "$INSTALLLOC" ]; then
        echo "0x5 Invalid Install Directory"
        exit 5
    fi
    rm -f $INSTALLLOC/BeamMP-Launcher &>/dev/null
    rm -f $INSTALLLOC/BeamMP.ico &>/dev/null
    DOECHO 5
    touch $INSTALLLOC/BeamMP-Launcher
    cat "$0" | tail -n +FILELINELENGTH0 | head -n OUTFILELINELENGTH0 > $INSTALLLOC/BeamMP-Launcher
    chmod +x $INSTALLLOC/BeamMP-Launcher &>/dev/null
    DOECHO 35
    touch $INSTALLLOC/BeamMP.ico
    cat "$0" | tail -n +FILELINELENGTH1 | head -n OUTFILELINELENGTH1 > $INSTALLLOC/BeamMP.ico
    DOECHO 65
    touch $INSTALLLOC/BeamMP.desktop
    cat "$0" | tail -n +FILELINELENGTH2 | head -n OUTFILELINELENGTH2 > $INSTALLLOC/BeamMP.desktop
    sed -i "s/SETUPINSTALLLOCATION/$(echo "$INSTALLLOC" | sed "s/\\//\\\\\\//g")/g" $INSTALLLOC/BeamMP.desktop
    DOECHO 95
    if [ -n "$BINSHORT" ]; then
        rm $BINSHORT/BeamMP-Launcher &>/dev/null
        DOLINK $INSTALLLOC/BeamMP-Launcher $BINSHORT/BeamMP-Launcher
        chmod +x $BINSHORT/BeamMP-Launcher
    fi
    if [ -n "$DESKTOPSHORT" ]; then
        rm $DESKTOPSHORT/BeamMP.desktop &>/dev/null
        cp $INSTALLLOC/BeamMP.desktop $DESKTOPSHORT/BeamMP.desktop # Common practice to cp, not link
    fi
    if [ -n "$STARTMENUSHORT" ]; then
        rm $STARTMENUSHORT/BeamMP.desktop &>/dev/null
        DOLINK $INSTALLLOC/BeamMP.desktop $STARTMENUSHORT/BeamMP.desktop
        if command -v update-desktop-database >/dev/null; then
            update-desktop-database $STARTMENUSHORT
        fi
    fi
    DOECHO 100
}

if [ -n "$WHIPTAILF" ]; then
    clear
    echo "$MENU5..."
    echocommand=""
    PERFORM_SETUP
    echo "Finished installing BeamMP"
else
    {
        PERFORM_SETUP
    } | whiptail --title "$TITLE" --gauge "$MENU5..." 0 0 0
fi

if [ -n "$WHIPTAILF" ]; then
    while true; do
        clear
        echo -e "$MENU6\n"
        read -p "$OPTION51 [$YESBUTTON/$NOBUTTON]> " RESPONSE
        case "${RESPONSE,,}" in
            "${YESBUTTON,,}" ) cd $INSTALLLOC; "$INSTALLLOC/BeamMP-Launcher"; exit $?; break;;
            "${NOBUTTON,,}" ) echo "$REJECT0"; break;;
            * ) clear;;
        esac
    done
else
    eval DONES=($(whiptail --nocancel --notags --title "$TITLE" --checklist "$MENU6" 0 0 0 "1" "$OPTION51" ON 3>&1 1>&2 2>&3))
    for DONE in "${DONES[@]}"; do
        if [[ "$DONE" == "1" ]]; then
            cd $INSTALLLOC
            "$INSTALLLOC/BeamMP-Launcher"
            exit $?
        else
            echo "$REJECT0"
        fi
    done
fi
