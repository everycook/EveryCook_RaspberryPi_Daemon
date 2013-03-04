<?php

namespace EveryCook;

use Wrench\Connection;
use Wrench\Socket\ServerClientSocket;
use Wrench\Exception\ConnectionException;

use Wrench\Protocol\Protocol;
use Wrench\Protocol\ServerConnectProtocol;
use Wrench\Payload\Payload;
use Wrench\Payload\PayloadHandler;

use \Exception;
use \InvalidArgumentException;

class EveryCookConnection extends Connection {
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