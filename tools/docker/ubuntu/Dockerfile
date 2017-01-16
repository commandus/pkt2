#
# Dockerfile for Ubuntu 16.04
# You must provide host name resolution for "db" like this:
#   docker run --add-host db:84.237.104.41 -t ubuntu:16.04 top
# "db" is PostgresSQL 9.5 server available at 5432 TCP port.
# You must grant access database client connection as described below. 
# PostgreSQL server check config:
#   /etc/postgresql/9.5/main/postgresql.conf: 
#     listen_addresses = '*' in the
#   /etc/postgresql/9.5/main/pg_hba.conf 
#     host onewayticket onewayticket 0.0.0.0/0 md5
# Then:
#   /etc/init.d/postgresql restart
# See also: how to install PostreSQL on Docker https://docs.docker.com/engine/examples/postgresql_service/
#
# How to run:
#   docker run -P --rm --add-host db:10.0.2.15 -w "/root" -t ubuntu:16.04 /usr/local/bin/onewayticketsvc --host db --user onewayticket --database onewayticket --password 123456 --k cert/server.key -c cert/server.crt -K cert/ca.key -C cert/ca.crt -vvvv
#
# Tips:
#   Add ifconfig, ping, vi:
#   RUN apt-get install -y net-tools iputils-ping vim
#
# Notices:
#   - Do not provide production database in --add-host db:
#     =========================
#     ALL DATA WILL BE DROPPED!
#     =========================
#   - Do not use Ubuntu 16.10- there no PostreSQL client for.
#
# Set database user password below
# 

FROM ubuntu:16.04
MAINTAINER andrei.i.ivanov@commandus.com

# External PostgreSQL
ENV POSTGRES_VERSION="9.5" POSTGRES_SCHEMA="onewayticket" POSTGRES_PORT="5432" POSTGRES_USER_NAME="onewayticket" POSTGRES_USER_PASSWORD="123456"
# service port
ENV GRPC_PORT="50051"

# first install lsb-release tool and add-apt-repository tool, wget
RUN apt-get update && apt-get install -y lsb-release software-properties-common python-software-properties wget curl

# add PostgreSQL 9.5 repository for specified version
RUN echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg $POSTGRES_VERSION" >> /etc/apt/sources.list.d/pgdg.list
# add repository key
RUN wget -q https://www.postgresql.org/media/keys/ACCC4CF8.asc -O - | apt-key add -
 
# update to get a new GCC 
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y 

# install all other dependencies
RUN apt-get update && apt-get install -y g++-6 && apt-get install -y --allow-unauthenticated \
  gcc-5-plugin-dev \
  gcc-6-plugin-dev \
  cmake \
  build-essential \
  autoconf \
  automake \
  libtool \
  patch \
  git \
  libpq-dev \
  postgresql-client \
  vim

# create an user. Just in case.
RUN useradd -m -d /home/onewayticket onewayticket

# psql default user & password to connect database
RUN echo db:$POSTGRES_PORT:*:$POSTGRES_USER_NAME:$POSTGRES_USER_PASSWORD > /root/.pgpass && chmod 0600 /root/.pgpass
RUN echo db:$POSTGRES_PORT:*:$POSTGRES_USER_NAME:$POSTGRES_USER_PASSWORD > /home/onewayticket/.pgpass && chown onewayticket:onewayticket /home/onewayticket/.pgpass && chmod 0600 /home/onewayticket/.pgpass

# copy dist tarball
RUN mkdir -p /root/src/
COPY docker/onewayticket-*.tar.gz /root/src 
RUN cd /root/src && tar xvfz onewayticket-*.tar.gz && rm onewayticket-*.tar.gz && mv onewayticket-* onewayticket-cpp
# copy certificates to home directories
RUN cp -r /root/src/onewayticket-cpp/cert /root && cp -r /root/src/onewayticket-cpp/cert /home/onewayticket

# Remove OpenSSL dev before re-build
# RUN apt-get remove -y libssl-dev
# download libs & build dependencies
RUN cd /root/src/onewayticket-cpp/tools && ./mkdependencies

# build sources 
RUN cd /root/src/onewayticket-cpp && ./configure && make && make install && make clean

# run service from the directory with cert subdirectory
# RUN cd /root/src/onewayticket-cpp && onewayticketsvc --user $POSTGRES_USER_NAME --database $POSTGRES_SCHEMA --password $POSTGRES_USER_PASSWORD --port $POSTGRES_PORT --k cert/server.key -c cert/server.crt -K cert/ca.key -C cert/ca.crt -vvvv

# remove build tools if needed
# RUN apt-get remove --purge -y build-essential g++-6 autoconf automake libtool curl software-properties-common python-software-properties
  
# Expose the onewayticket port
EXPOSE $GRPC_PORT
