<?php

namespace EveryCook\Payload;

use EveryCook\Frame\ServerConnectFrame;
use Wrench\Exception\PayloadException;

/**
 * Gets a HyBi payload
 * @author Dominic
 *
 */
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