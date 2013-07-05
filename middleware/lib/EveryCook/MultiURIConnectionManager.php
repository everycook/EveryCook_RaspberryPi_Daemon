<?php

namespace EveryCook;

use Wrench\Server;
use Wrench\Connection;
use Wrench\ConnectionManager;
use Wrench\Protocol\Protocol;
use Wrench\Resource;
use Wrench\Util\Configurable;
use Wrench\Exception\Exception as WrenchException;
use Wrench\Exception\CloseException;
use \Exception;
use \Countable;

class MultiURIConnectionManager extends ConnectionManager
{
    /**
     * Master socket
     *
     * @var Socket
     */
    protected $socket;
	
	/**
	 * array of resources to simple check it's a server
	 */
	protected $serverSocketRecources = array();

    /**
     * @see Wrench\Socket.Socket::configure()
     *   Options include:
     *     - timeout_select          => int, seconds, default 0
     *     - timeout_select_microsec => int, microseconds (NB: not milli), default: 200000
     */
    protected function configure(array $options)
    {
        $options = array_merge(array(
            'connection_class'        => 'EveryCook\MultiURIConnection',
        ), $options);
		
        parent::configure($options);
    }

    /**
     * Gets the application associated with the given path
     *
     * @param string $path
     */
    public function getApplicationForPath($path)
    {
        $path = ltrim($path, '/');
		@list($path, $params) = explode("?", $path, 2);
        return $this->server->getApplication($path);
    }
	
    /**
     * Configures the main server socket
     *
     * @param string $uri
     */
    protected function configureMasterSocket()
    {
        $class   = $this->options['socket_master_class'];
        $options = $this->options['socket_master_options'];
		
		$uris = $this->getUri();
		
		$this->socket = array();
		for ($i=0; $i<count($uris); ++$i){
			$this->socket[$i] = new $class($uris[$i], $options);
		}
    }

    /**
     * Listens on the main socket
     *
     * @return void
     */
    public function listen()
    {
		$this->serverSocketRecources = array();
		for ($i=0; $i<count($this->socket); ++$i){
			$this->socket[$i]->listen();
			$this->resources[$this->socket[$i]->getResourceId()] = $this->socket[$i]->getResource();
			$this->serverSocketRecources[$this->socket[$i]->getResourceId()] = $i;
		}
    }

    /**
     * Gets all resources
     *
     * @return array<int => resource)
     */
    protected function getAllResources()
    {
		//master sockets allready added in listen method...
		return $this->resources;
    }

    /**
     * Select and process an array of resources
     *
     * @param array $resources
     */
    public function selectAndProcess()
    {
        $read             = $this->resources;
        $unused_write     = null;
        $unsued_exception = null;

        stream_select(
            $read,
            $unused_write,
            $unused_exception,
            $this->options['timeout_select'],
            $this->options['timeout_select_microsec']
        );

        foreach ($read as $socket) {
			if (isset($this->serverSocketRecources[$this->resourceId($socket)])) {
                $this->processMasterSocket($socket);
            } else {
                $this->processClientSocket($socket);
            }
        }
    }

    /**
     * Process events on the master socket ($this->socket)
     *
     * @return void
     */
    protected function processMasterSocket($socketResource = null)
    {
        $new = null;
		
		if ($socketResource == null){
			echo "processMasterSocket, socket is null\r\n";
			$socket = $this->socket[0];
		} else {
			$socket = $this->socket[$this->serverSocketRecources[$this->resourceId($socketResource)]];
		}

        try {
            $new = $socket->accept();
        } catch (Exception $e) {
            $this->server->log('Socket error: ' . $e, 'err');
            return;
        }

        $connection = $this->createConnection($new, $socket);
        $this->server->notify(Server::EVENT_SOCKET_CONNECT, array($new, $connection));
    }

    /**
     * Creates a connection from a socket resource
     *
     * The create connection object is based on the options passed into the
     * constructor ('connection_class', 'connection_options'). This connection
     * instance and its associated socket resource are then stored in the
     * manager.
     *
     * @param resource $resource A socket resource
     * @return Connection
     */
    protected function createConnection($resource, $masterSocket = null)
    {
        $connection = parent::createConnection($resource);
		
		if(method_exists($connection, 'setUri')) {
			$uris = $this->getUri();
			$index = 0;
			if ($masterSocket == null){
				$masterSocket = $this->socket[0];
			}
			if (isset($this->serverSocketRecources[$masterSocket->getResourceId()])) {
				$index = $this->serverSocketRecources[$masterSocket->getResourceId()];
			}
			$uri = $uris[$index];
			$connection->setUri($uri);
		}
		
        return $connection;
    }
	
    /**
     * Gets the connection manager's listening URI
     *
     * @return string
     */
    public function getUri()
    {
		$uris = $this->server->getUri();
		if (is_array($uris)){
			return $uris;
		} else {
			return array($uris);
		}
    }

    /**
     * Logs a message
     *
     * @param string $message
     * @param string $priority
     */
    public function log($message, $priority = 'info')
    {
        $this->server->log(sprintf(
            '%s: %s',
            __CLASS__,
            $message
        ), $priority);
    }

    /**
     * @return \Wrench\Server
     */
    public function getServer()
    {
        return $this->server;
    }

    /**
     * Removes a connection
     *
     * @param Connection $connection
     */
    public function removeConnection(Connection $connection)
    {
        $socket = $connection->getSocket();

        if ($socket->getResource()) {
            $index = $socket->getResourceId();
        } else {
            $index = array_search($connection, $this->connections);
        }

        if (!$index) {
            $this->log('Could not remove connection: not found', 'warning');
        }

        unset($this->connections[$index]);
        unset($this->resources[$index]);

        $this->server->notify(
            Server::EVENT_SOCKET_DISCONNECT,
            array($connection->getSocket(), $connection)
        );
    }
}
