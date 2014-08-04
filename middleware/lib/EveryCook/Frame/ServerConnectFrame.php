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

namespace EveryCook\Frame;

use Wrench\Protocol\Protocol;
use Wrench\Frame\Frame;
use Wrench\Exception\FrameException;
use \InvalidArgumentException;

class ServerConnectFrame extends Frame
{
    const LENGTH_INFO_LENGTH = 2;
    const TYPE_INFO_LENGTH = 1;
	
    /**
     * @see Wrench\Frame.Frame::encode()
     */
    public function encode($payload, $type = Protocol::TYPE_TEXT, $masked = false)
    {
        if (!is_int($type) || !in_array($type, Protocol::$frameTypes)) {
            throw new InvalidArgumentException('Invalid frame type');
        }
		/*
        if (!is_int($type) || ($type != Protocol::TYPE_TEXT && $type != Protocol::TYPE_CLOSE)) {
            throw new InvalidArgumentException('Invalid frame type');
        }
		*/
		
        $this->type = $type;
        $this->payload = $payload;
        $this->length = strlen($this->payload);

        $this->buffer = "";
        $this->buffer .= pack('n', $this->length);
		$this->buffer .= chr(0xFF & $this->type);
		$this->buffer .= $this->payload;
		
        return $this;
    }

    public function receiveData($data)
    {
        if ($this->getBufferLength() <= 1) {
            $this->length = null;
        }
        parent::receiveData($data);
    }

    /**
     * @see Wrench\Frame.Frame::getExpectedDataLength()
     */
    protected function getExpectedBufferLength()
    {
        return $this->getLength() + self::LENGTH_INFO_LENGTH + self::TYPE_INFO_LENGTH;
	
    }

    /**
     * @see Wrench\Frame.Frame::getLength()
     */
    public function getLength()
    {
        if (!$this->length) {
			if ($this->getBufferLength() < self::LENGTH_INFO_LENGTH) {
				throw new FrameException('Cannot get extended length: need more data');
			}
			$this->length = (ord($this->buffer[0]) << 8) + ord($this->buffer[1]);
        }
        return $this->length;
    }
	
    /**
     * @see Wrench\Frame.Frame::decodeFramePayloadFromBuffer()
     */
    protected function decodeFramePayloadFromBuffer()
    {
        $payload = substr($this->buffer, self::LENGTH_INFO_LENGTH + self::TYPE_INFO_LENGTH);
		
        $this->payload = $payload;
    }

    /**
     * @see Wrench\Frame.Frame::isFinal()
     */
    public function isFinal()
    {
		return true;
    }

    /**
     * @throws FrameException
     * @see Wrench\Frame.Frame::getType()
     */
    public function getType()
    {
		//return Protocol::TYPE_TEXT;
        if (!isset($this->buffer[2])) {
            throw new FrameException(strlen($this->buffer));
        }
		
		$type = ord($this->buffer[2]);
        
        if (!in_array($type, Protocol::$frameTypes)) {
            throw new FrameException('Invalid payload type');
        }
		
        return $type;
    }
}