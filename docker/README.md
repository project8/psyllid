# Docker directory

## Broker in a container

If you'd like to run Psyllid on your host machine but use Docker to run the RabbitMQ broker, use the `docker-compose-broker.yaml` file.  It will start a broker in a container and expose to the host the ports 5672 (for sending and receiving AMQP messages) and 15672 (for accessing the web GUI).

First start the broker in one terminal (or use `-d` to start as a daemon):

    > docker compose -f docker-compose-broker.yaml up

From the host the broker will be accessible at `localhost:5672`.  Next you'll start Psyllid:

    > /path/to/psyllid --auth-file authentications.json -c ../examples/fmt_1ch_socket.yaml

You'll need to use your host's copy of `dl-agent` to interact with Psyllid:

    > /path/to/dl-agent --auth-file authentications.json cmd psyllid ...

## Everything in containers

If you'd like to run Psyllid in a container in addition to RabbitMQ running in a container, use the `docker-compose-full.yaml` file.  It will first start a broker, then start Psyllid.

You can use the environment variable `IMG_TAG` to set the container tag for Psyllid.  The full container specification used will be `ghcr.io/project8/psyllid:${IMG_TAG}`.  If that variable isn't set, `latest` will be used as the default.

You can specify the configuration file used with the `PSYLLID_CONFIG` environment variable.  The `examples` directory is mounted into the container at `/configs`, so you can use anything in that directory without modifying the compose file.  If you want to use another file, you can add it to the `examples` directory or you can modify the compose file to mount that config file into the container.  You'll need to specify the full path to the config file in `PSYLLID_CONFIG`.

The Psyllid container will use the healthcheck for the broker to ensure that the broker is available for connections before it starts.

You can start everything with:

    > docker compose -f docker-compose-full.yaml up

You can prepend that command with an environment variable setting if you want it to apply just to the command you're running:

    > IMG_TAG=my_tag docker compose -f docker-compose-full.yaml up

Side note: the `psyllid` command in `docker-compose-full.yaml` overrides the broker specification to set it to be `rabbit-broker`.  This is because the auth file and the example config file both set the broker to `localhost` (side-side note: the config file overrides the auth file, but the command line overrides both of them).

You'll need to use `dl-agent` to control Psyllid.  This can be done from the host since port 5672 of the broker container is exposed to the host, or by using `docker exec` to get a command-line prompt in the Psyllid container:

    > docker exec -it docker-psyllid-1 bash
    