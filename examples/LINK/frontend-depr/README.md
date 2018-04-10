# RTX-LINK
the tier-2 cloud historian replication service for applications build with epanet-rtx. this tool runs as a Node application, and can be packaged in Electron. requires an InfluxDB server installation to store the data.

# Docker Installation
The following instructions assume you are installing this application as a Docker image, either on a Linux server or on an embedded system like Raspberry Pi. 

```
mkdir RTX_LINK_DOCKER && cd RTX_LINK_DOCKER
curl https://raw.githubusercontent.com/OpenWaterAnalytics/RTX-LINK/master/docker/odbcinst.ini > odbcinst.ini
curl https://raw.githubusercontent.com/OpenWaterAnalytics/RTX-LINK/master/docker/Dockerfile-rpi > Dockerfile
# ... or download Dockerfile-rpi for raspberry install
docker build -t rtx-link -f Dockerfile .
docker run --restart always -d -p 3131:3131 -p "8585:8585" -v "$(pwd)/RTX_LINK_DOCKER/cfg:/root/rtx_link" --name link-app rtx-link 
```

