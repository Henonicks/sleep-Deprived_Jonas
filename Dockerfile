FROM brainboxdotcc/dpp:latest

WORKDIR /usr/src/sleepless_jonas

COPY . .

WORKDIR /usr/src/sleepless_jonas/build

RUN cmake ..
RUN make -j$(nproc)

ENTRYPOINT [ "/usr/src/sleepless_jonas/build/sleepless_jonas" ]
