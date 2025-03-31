#!/bin/sh

SCRIPT_DIR=$(cd $(dirname $0); pwd)

###### MODIFY THIS VARIABLE TO CHANGE THE INSTALLATION DIRECTORY ###########
STM32_INSTALL_DIR="${HOME}/STMicroelectronics/STM32Cube/STM32CubeProgrammer"

STM32_PROGRAMMER_VERSION="2-19-0"
STM32_PROGRAMMER_ZIP="en.stm32cubeprg-lin-v${STM32_PROGRAMMER_VERSION}.zip"
STM32_PROGRAMMER="${STM32_INSTALL_DIR}/bin/STM32_Programmer_CLI"

echo "Checking for STM32 Programmer..."
if [ -f ${STM32_PROGRAMMER} ]; then
    echo "STM32 Programmer is already installed."
    exit 0
fi

echo "Checking for STM32 Programmer ZIP..."
echo "${SCRIPT_DIR}/${STM32_PROGRAMMER_ZIP}"

if [ ! -f ${SCRIPT_DIR}/${STM32_PROGRAMMER_ZIP} ]; then
    printf "[ERROR] STM32 Programmer ZIP not found.\n"
    printf "\tPlease download the STM32 Programmer from the following link:\n"
    printf "\thttps://www.st.com/en/development-tools/stm32cubeprog.html\n"
    printf "\tThen, place the downloaded file in 'scripts' directory.\n"
    exit 1
fi

echo "Unzipping STM32 Programmer..."
TMP_DIR="${SCRIPT_DIR}/tmp"
unzip ${SCRIPT_DIR}/${STM32_PROGRAMMER_ZIP} -d ${TMP_DIR}

sed -i "s|<installpath>.*</installpath>|<installpath>${STM32_INSTALL_DIR}</installpath>|" ${SCRIPT_DIR}/stm32cubeprg-auto-install.xml

echo "Installing STM32 Programmer in ${STM32_INSTALL_DIR}..."
cd ${TMP_DIR}
./SetupSTM32CubeProgrammer-2.19.0.linux ../stm32cubeprg-auto-install.xml
cd ${SCRIPT_DIR}
rm -rf ${TMP_DIR}

