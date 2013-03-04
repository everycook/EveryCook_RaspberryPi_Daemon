<?php

namespace EveryCook\Protocol;

use EveryCook\Payload\ServerConnectPayload;

use Wrench\Exception\ConnectionException;

use Wrench\Protocol\Protocol;
use \InvalidArgumentException;

/**
 * @see http://tools.ietf.org/html/rfc6455#section-5.2
 */
class ServerConnectProtocol extends Protocol
{
    /**
     * @see Wrench\Protocol.Protocol::getPayload()
     */
    public function getPayload(){
        return new ServerConnectPayload();
    }
	
    /**
     * @see Wrench\Protocol.Protocol::getPayload()
     */
    public function getVersion(){
		return 1;
	}

    /**
     * @see Wrench\Protocol.Protocol::getPayload()
     */
    public function acceptsVersion($version){
		return "1";
	}
	
    /**
     * @see Wrench\Protocol.Protocol::getResponseError()
     */
	public function getResponseError($e, array $headers = array()){
		return $this->getCloseFrame($e);
    }
}