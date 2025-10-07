# WebRTC Setup and Configuration Guide
This is the instrumented WebRTC used by [Domino-IMC](https://doi.org/10.1145/3730567.3764434), detailed loggings can be found in [Wiki Page](https://github.com/PrincetonUniversity/libwebrtc-github/wiki). This guide provides step-by-step instructions for setting up, configuring, and running the instrumented WebRTC for research purposes.

## 1. Setup libwebrtc

### Install Dependencies

```bash
# Update package lists
sudo apt-get update

# Install basic requirements
sudo apt-get install -y git python3 python3-pip curl gnupg lsb-release
sudo apt-get install -y build-essential libssl-dev libffi-dev
sudo apt-get install -y ninja-build
```

### Setup depot_tools

```bash
# Clone Google's depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

# Optional: Add depot_tools to PATH
vim ~/.bashrc
# Add the following line at the end of the file:
# export PATH="$PATH:/path/to/depot_tools"
# My path is: /home/fanyi/proj_webrtc/depot_tools

# Apply the changes
source ~/.bashrc

# Verify the setup
which fetch
```

### Fetch WebRTC Source

```bash
# Create and enter directory
mkdir webrtc-checkout
cd webrtc-checkout

# Fetch WebRTC code
fetch --no-hooks webrtc

# Enter source directory
cd src

# Checkout specific version
git checkout master
git reset --hard 80899598775cb1bd13e298e050a608339b86f551
gclient sync --force --with_branch_heads --with_tags -r 80899598775cb1bd13e298e050a608339b86f551
```

### Configure Git Repository

```bash
# Add GitHub repo as a new remote
git remote add github git@github.com:Fanyi7362/libwebrtc-github.git

# Remove old remote
git remote remove origin

# Add new branch tracking github/main
git fetch github
git branch -D main
git checkout -b main --track github/main

# Remove old branch
git branch -D master
git pull github main
```

## 2. Compile libwebrtc

```bash
# Navigate to WebRTC source directory
cd ~/fanyi/proj_webrtc/webrtc-checkout/src

# Optional: Install build dependencies
./build/install-build-deps.sh

# Generate build files
gn gen out/Default
# For Mac: gn gen out/Default --args='target_os="mac" target_cpu="x64" is_debug=true'

# Copy client config
cp ./examples/peerconnection/client/linux/client.cfg ./client.cfg

# Build WebRTC
ninja -C out/Default
ninja -C out/Default -v

# Optional: Clean build artifacts while preserving configuration
gn clean out/Default
```

## 3. Git Repository Management

### Check Current Repository Status

```bash
# Check remote repositories
git remote -v

# Check branch tracking
git branch -vv

# Example output:
# * main 8089959877 [github/main] Remove private SRTP include
# master 8089959877 [origin/master] Remove private SRTP include
```

### Push Changes to GitHub

```bash
git checkout main
git add .
git commit -m "Your commit message"
git push github main
```

### Pull Changes from GitHub

```bash
git checkout main
git pull github main
```

## 4. Run PeerConnection Application

```bash
# Navigate to WebRTC source directory
cd ~/fanyi/proj_webrtc/webrtc-checkout/src

# Start the signaling server
./out/Default/peerconnection_server

# In separate terminals, run the clients
# Run caller (initiates connection)
vim ./client.cfg
# change to "disable_gui=true, is_caller=true"
# configure server IP and port, Stun IP and port
./out/Default/peerconnection_client

# Run callee (receives connection)
vim ./client.cfg
# change to "disable_gui=true, is_caller=false"
./out/Default/peerconnection_client
```