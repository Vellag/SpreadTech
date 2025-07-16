sudo apt-get update

base_file=%0
cd "$(dirname "$base_file")"

git clone https://github.com/Stollpy/Freenove-ADC-libs/
git clone https://github.com/WiringPi/WiringPi

cd Freenove-ADC-libs
sh ./build.sh
echo "ADCDevice library installed"

cd ..
cd WiringPi
sh ./build.sh
echo "WiringPi Library installed"

cd ..
g++ SpreadTech.cpp -o SpreadTech -lWiringPi -lADCDevice -lWiringPiDev
echo "All Done"