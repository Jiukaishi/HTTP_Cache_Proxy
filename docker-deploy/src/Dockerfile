FROM ubuntu
RUN mkdir /var/log/erss
RUN apt-get update && apt-get -y install g++ make
ADD . /var/log/erss
WORKDIR /var/log/erss/

ENTRYPOINT ["./run.sh"]