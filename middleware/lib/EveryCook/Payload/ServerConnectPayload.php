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

namespace EveryCook\Payload;

use EveryCook\Frame\ServerConnectFrame;
use Wrench\Payload\Payload;
use Wrench\Exception\PayloadException;

class ServerConnectPayload extends Payload
{
    /**
     * @see Wrench\Payload.Payload::getFrame()
     */
    protected function getFrame()
    {
        return new ServerConnectFrame();
    }
}