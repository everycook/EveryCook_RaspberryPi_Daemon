<?php
/*
This is the EveryCook Raspberry Pi daemon. It reads inputs from the EveryCook Raspberry Pi shield and controls the outputs.
EveryCook is an open source platform for collecting all data about food and make it available to all kinds of cooking devices.

This program is copyright (C) by EveryCook. Written by Samuel Werder.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

See GPLv3.htm in the main folder for details.
*/

namespace EveryCook\Protocol;

use EveryCook\Payload\ServerConnectPayload;

use Wrench\Exception\ConnectionException;

use Wrench\Protocol\Protocol;
use \InvalidArgumentException;

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