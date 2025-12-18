FROM alpine:latest

RUN apk add --no-cache util-linux

COPY worker.sh /worker.sh
RUN chmod +x /worker.sh

VOLUME ["/shared"]

CMD ["/worker.sh"]
