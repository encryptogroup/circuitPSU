git clone https://github.com/Visa-Research/volepsi.git
cd volepsi
git checkout 65fe0c7d814a7455939727285b028ab6f86ddcdb
python3 build.py -DVOLE_PSI_ENABLE_SSE=ON -DVOLE_PSI_ENABLE_BOOST=ON -DVOLE_PSI_ENABLE_GMW=ON -DVOLE_PSI_ENABLE_CPSI=ON -DVOLE_PSI_ENABLE_RELIC=ON -DCMAKE_BUILD_TYPE=Release
mkdir -p ../libs/volepsi
python3 build.py --install=../libs/volepsi
# need to copy header file here
cd ..
cp config.h libs/volepsi/include/volePSI/config.h
