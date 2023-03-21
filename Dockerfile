FROM ubuntu:latest

RUN apt update

RUN apt install -y g++ build-essential cmake

RUN mkdir -p /home/app

ADD "./init.sh" "/home/app"

WORKDIR "/home/app"

CMD ["./init.sh"]
