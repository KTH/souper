
mkdir -p third_party/rabbitmq
cd third_party

ROOT=$(pwd)

cd rabbitmq


git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git amqpcpp


cd amqpcpp
mkdir -p build
cd build
cmake .. -DAMQP-CPP_BUILD_SHARED=ON -DAMQP-CPP_LINUX_TCP=ON -DCMAKE_PREFIX=$ROOT
cmake --build . --target install 