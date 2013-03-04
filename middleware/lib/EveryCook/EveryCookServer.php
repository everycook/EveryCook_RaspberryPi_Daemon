<?php

namespace EveryCook;

use Wrench\Server;

class EveryCookServer extends Server{
    /**
     * Constructor
     *
     * @param string $uri
     * @param array $options
     */
    public function __construct($uri, array $options = array()){
        parent::__construct($uri, $options);
    }

    /**
     * @see Wrench.Server::configure()
     */
    protected function configure(array $options){
		/*
        $options = array_merge(array(
            'check_origin'        => true,
            'allowed_origins'     => array(),
            'origin_policy_class' => 'Wrench\Listener\OriginPolicy',
            'rate_limiter_class'  => 'Wrench\Listener\RateLimiter'
        ), $options);
		*/
        parent::configure($options);
    }
	
    /**
     * @see Wrench.Server::registerApplication()
     */
    public function registerApplication($key, $application){
		if(method_exists($application, 'onRegister')) {
			$application->onRegister($this);
		}
        $this->applications[$key] = $application;
    }
}