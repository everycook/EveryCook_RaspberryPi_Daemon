<?php
/*
This is the EveryCook Middleware. It's a Websocket-Server for communications between the EveryCook Daemon and the CookAssistant from EveryCook Recipe Database.
EveryCook is an open source platform for collecting all data about food and make it available to all kinds of cooking devices.

This program is copyright (C) by EveryCook. Written by Samuel Werder.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

See GPLv3.htm in the main folder for details.
*/

namespace EveryCook;

use Wrench\Connection;
use Wrench\Socket\ServerClientSocket;
use Wrench\Exception\ConnectionException;
use Wrench\ConnectionManager;

use Wrench\Protocol\Protocol;
use Wrench\Payload\Payload;
use Wrench\Payload\PayloadHandler;

use \Exception;
use \InvalidArgumentException;

class MultiURIConnection extends Connection {
	protected $uri;
	
    /**
     * Constructor
     *
     * @param Server $server
     * @param ServerClientSocket $socket
     * @param array $options
     * @throws InvalidArgumentException
     */
    public function __construct(
        ConnectionManager $manager,
        ServerClientSocket $socket,
        array $options = array()
    ) {
        parent::__construct($manager, $socket, $options);
    }
	
	public function setUri($uri){
		$this->uri = $uri;
	}
	
	public function getUri(){
		return $this->uri;
	}
	
    /**
     * Configures the client ID
     *
     * We hash the client ID to prevent leakage of information if another client
     * happens to get a hold of an ID. The secret *must* be lengthy, and must
     * be kept secret for this to work: otherwise it's trivial to search the space
     * of possible IP addresses/ports (well, if not trivial, at least very fast).
     */
    protected function configureClientId()
    {
        $message = sprintf(
            '%s:uri=%s&ip=%s&port=%s',
            $this->options['connection_id_secret'],
            rawurlencode($this->uri),
            rawurlencode($this->ip),
            rawurlencode($this->port)
        );

        $algo = $this->options['connection_id_algo'];

        if (extension_loaded('gmp')) {
            $hash = hash($algo, $message, true);
            $hash = gmp_strval(gmp_init($hash, 16), 62);
        } else {
            // @codeCoverageIgnoreStart
            $hash = hash($algo, $message);
            // @codeCoverageIgnoreEnd
        }

        $this->id = $hash;
    }

}