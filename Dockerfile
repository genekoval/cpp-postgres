FROM git.aurora.aur/aurora/cpp

RUN pacman --noconfirm -S postgresql

COPY . .

RUN cmake --workflow --preset=docker

CMD ctest --preset docker
