FROM ubuntu:24.04

# Prevent apt from showing interactive prompts and configuration dialogs during package installation
# This is a Docker best practice to ensure builds are fully non-interactive
ENV DEBIAN_FRONTEND=noninteractive

# Install system prerequisites and tools
RUN apt-get update && \
    # Install wget (for downloading), apt-transport-https (for https apt sources), python3, git, pre-commit and cmake
    apt-get install -y wget apt-transport-https python3 git pre-commit cmake make && \
    # Create python symlink so 'python' command works
    ln -sf /usr/bin/python3 /usr/bin/python && \
    # Clean up apt cache to reduce image size
    rm -rf /var/lib/apt/lists/*

# Install devkitpro-pacman manually instead of using the install script
# The official install script (https://apt.devkitpro.org/install-devkitpro-pacman)
# runs apt-get install without -y flags, causing interactive prompts during build
# We replicate the script's steps here with proper -y flags for non-interactive installation
RUN mkdir -p /usr/local/share/keyring/ && \
    # Download and store devkitPro GPG key
    wget -U "dkp apt" -O /usr/local/share/keyring/devkitpro-pub.gpg https://apt.devkitpro.org/devkitpro-pub.gpg && \
    # Add devkitPro apt repository
    echo "deb [signed-by=/usr/local/share/keyring/devkitpro-pub.gpg] https://apt.devkitpro.org stable main" > /etc/apt/sources.list.d/devkitpro.list && \
    # Update package list with new repository and install devkitpro-pacman
    apt-get update && \
    apt-get install -y devkitpro-pacman && \
    # Clean up apt cache to reduce image size
    rm -rf /var/lib/apt/lists/*

# Set up devkitPro environment variables
ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH

# Install GBA development toolchain using devkitPro pacman
# Create /etc/mtab symlink required by pacman for filesystem mount point detection
RUN ln -sf /proc/self/mounts /etc/mtab && \
    dkp-pacman -Syu --noconfirm && \
    dkp-pacman -S --noconfirm gba-dev

# Install upstream mGBA 0.10.5 release
# xvfb provides virtual framebuffer for headless GUI testing
# pulseaudio provides null audio sink for headless audio
RUN apt-get update && \
    apt-get install -y xvfb pulseaudio sudo && \
    wget -O /tmp/mgba.tar.xz https://github.com/mgba-emu/mgba/releases/download/0.10.5/mGBA-0.10.5-ubuntu64-noble.tar.xz && \
    mkdir /tmp/mgba && \
    tar -xf /tmp/mgba.tar.xz -C /tmp/mgba && \
    apt-get install -y /tmp/mgba/*/*.deb && \
    rm -rf /var/lib/apt/lists/* /tmp/mgba /tmp/mgba.tar.xz

# Grant passwordless sudo to the default ubuntu user
RUN echo "ubuntu ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/ubuntu && \
    chmod 0440 /etc/sudoers.d/ubuntu

USER ubuntu

