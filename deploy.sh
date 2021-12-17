#!/bin/bash
CWD=$PWD
echo -e "\n\nInstalando commons Library, o eso creo...\n\n"
COMMONS="so-commons-library"
git clone "https://github.com/sisoputnfrba/${COMMONS}.git" $COMMONS
cd $COMMONS

sudo make uninstall
make all
sudo make install
ls -a

cd ..
PRUEBACARPINCHO="carpinchos-pruebas"
echo -e "\n\n Instalando las pruebas de carpinchoide... \n\n"
git clone "https://github.com/sisoputnfrba/carpinchos-pruebas.git" $PRUEBACARPINCHO
