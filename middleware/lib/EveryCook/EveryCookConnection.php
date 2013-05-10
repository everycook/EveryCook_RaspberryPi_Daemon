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

use EveryCook\Protocol\ServerConnectProtocol;

use EveryCook\MultiURIConnection;
use Wrench\Socket\ServerClientSocket;
use Wrench\Exception\ConnectionException;
use Wrench\ConnectionManager;

use Wrench\Protocol\Protocol;
use Wrench\Payload\Payload;
use Wrench\Payload\PayloadHandler;

use \Exception;
use \InvalidArgumentException;

class EveryCookConnection extends MultiURIConnection {
    protected $isServerConnection = false;
	
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
	
	public function handshakeServerConnect($data){
		$this->isServerConnection = true;
		$lines = explode("\r\n", $data);
		$firstLine = array_shift($lines);
		$headers = array();
		foreach ($lines as $header) {
			$parts = explode(': ', $header, 2);
			if (count($parts) == 2) {
				 list($name, $value) = $parts;
				 $headers[$name] = $value;
			}
		}
		
		$this->options['protocol'] = new ServerConnectProtocol();
		$this->configureProtocol();
		
        $this->payloadHandler = new PayloadHandler(
            array($this, 'handleServerConnectPayload'),
            $this->options
        );
		$this->payloadHandler->options['protocol'] = $this->options['protocol'];
		$this->payloadHandler->configureProtocol();
		
		$this->application = $this->manager->getApplicationForPath($headers['application']);
		
		if (method_exists($this->application, 'onServerConnect')) {
			$this->application->onServerConnect($this);
		}
		$this->handshaked = true;
	}
	
    /**
     * @see Wrench.Connection::handshake()
     */
    public function handshake($data)
    {
		try {
			if (strlen($data)>=13 && substr($data, 0, 13) == 'ServerConnect'){
				$this->handshakeServerConnect($data);
			} else {
				parent::handshake($data);
			}
		} catch(Exception $e) {
			$this->log('Exception in handshake: ' . $e, 'error');
			$this->close();
		}
    }
	
    /**
     * @see Wrench.Connection::handlePayload()
     */
    public function handleServerConnectPayload(Payload $payload)
    {
        $app = $this->getClientApplication();
		
        //$this->log('Handling payload: data:"' . $payload->getPayload() . '", RemainingData:' . $payload->getRemainingData(), 'debug');
		
        switch ($type = $payload->getType()) {
            case Protocol::TYPE_TEXT:
                if (method_exists($app, 'onServerConnectData')) {
                    $app->onServerConnectData($payload, $this);
                }
                return;
			
            case Protocol::TYPE_CLOSE:
                $this->log('Close frame received', 'notice');
                $this->close();
                $this->log('Disconnected', 'info');
            break;
			
            default:
                throw new ConnectionException('Unhandled payload type');
        }
    }

}