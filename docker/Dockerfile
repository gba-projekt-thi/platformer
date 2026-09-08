FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y wget apt-transport-https gnupg python3 git pre-commit bear cmake make && \
    ln -sf /usr/bin/python3 /usr/bin/python && \
    rm -rf /var/lib/apt/lists/*

# Install clang-tidy-19 / clang-format-19 from the official LLVM apt repository.
# Registered via update-alternatives so unversioned clang-tidy/clang-format calls
# (used by the pre-commit hooks) keep working without hardcoding the version there.
RUN mkdir -p /usr/local/share/keyring/ && \
    wget -O /usr/local/share/keyring/llvm-snapshot.gpg.key https://apt.llvm.org/llvm-snapshot.gpg.key && \
    gpg --dearmor -o /usr/local/share/keyring/llvm-snapshot.gpg /usr/local/share/keyring/llvm-snapshot.gpg.key && \
    rm /usr/local/share/keyring/llvm-snapshot.gpg.key && \
    echo "deb [signed-by=/usr/local/share/keyring/llvm-snapshot.gpg] http://apt.llvm.org/noble/ llvm-toolchain-noble-19 main" > /etc/apt/sources.list.d/llvm-19.list && \
    apt-get update && \
    apt-get install -y clang-tidy-19 clang-format-19 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-19 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-19 100 && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/local/share/keyring/ && \
    wget -U "dkp apt" -O /usr/local/share/keyring/devkitpro-pub.gpg https://apt.devkitpro.org/devkitpro-pub.gpg && \
    echo "deb [signed-by=/usr/local/share/keyring/devkitpro-pub.gpg] https://apt.devkitpro.org stable main" > /etc/apt/sources.list.d/devkitpro.list && \
    apt-get update && \
    apt-get install -y devkitpro-pacman && \
    rm -rf /var/lib/apt/lists/*

ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH

RUN ln -sf /proc/self/mounts /etc/mtab && \
    dkp-pacman -Syu --noconfirm && \
    dkp-pacman -S --noconfirm gba-dev

# Version-independent symlink to the installed devkitARM GCC C++ headers,
# referenced by .clang-tidy's isystem paths. Avoids hardcoding the GCC
# version, which changes whenever dkp-pacman updates the toolchain.
RUN GCC_CXX_DIR="$(basename "$(find /opt/devkitpro/devkitARM/arm-none-eabi/include/c++ -mindepth 1 -maxdepth 1 -type d | head -n1)")" && \
    ln -sf "$GCC_CXX_DIR" /opt/devkitpro/devkitARM/arm-none-eabi/include/c++/current

RUN apt-get update && \
    apt-get install -y xvfb pulseaudio sudo && \
    wget -O /tmp/mgba.tar.xz https://github.com/mgba-emu/mgba/releases/download/0.10.5/mGBA-0.10.5-ubuntu64-noble.tar.xz && \
    mkdir /tmp/mgba && \
    tar -xf /tmp/mgba.tar.xz -C /tmp/mgba && \
    apt-get install -y /tmp/mgba/*/*.deb && \
    rm -rf /var/lib/apt/lists/* /tmp/mgba /tmp/mgba.tar.xz

RUN echo "ubuntu ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/ubuntu && \
    chmod 0440 /etc/sudoers.d/ubuntu

USER ubuntu