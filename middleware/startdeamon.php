#!/usr/bin/env php
<?php

/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */

ini_set('display_errors', 1);
error_reporting(E_ALL);

require(__DIR__ . '/lib/SplClassLoader.php');

$classLoader = new SplClassLoader('Wrench', __DIR__ . '/lib');
$classLoader->register();

$classLoader = new SplClassLoader('EveryCook', __DIR__ . '/lib');
$classLoader->register();

set_include_path(get_include_path() . PATH_SEPARATOR . '/var/www/db_wsd/protected/components/' . PATH_SEPARATOR . '/var/www/db_wsd/protected/models/');

//$yii = $_SERVER['DOCUMENT_ROOT'].'/yii/framework/yii.php';
//$yii = 'E:/htdocs/EveryCook/lib/yii/framework/yii.php';
$yii='/var/www/db_wsd/lib/yii/framework/yii.php';
require_once($yii);
//YiiBase::registerAutoloader($my_autoload, true);

/*
require(__DIR__ . '/EveryCookApplication.php');
require(__DIR__ . '/EveryCookServer.php');
require(__DIR__ . '/EveryCookConnection.php');
*/

$server = new \EveryCook\EveryCookServer('ws://10.0.0.1:8000/', array(
    'allowed_origins'            => array(
        'mysite.localhost'
    ),
// Optional defaults:
//     'check_origin'               => true,
//     'connection_manager_class'   => 'Wrench\ConnectionManager',
     'connection_manager_options' => array(
//         'timeout_select'           => 0,
//         'timeout_select_microsec'  => 200000,
//         'socket_master_class'      => 'Wrench\Socket\ServerSocket',
//         'socket_master_options'    => array(
//             'backlog'                => 50,
//             'ssl_cert_file'          => null,
//             'ssl_passphrase'         => null,
//             'ssl_allow_self_signed'  => false,
//             'timeout_accept'         => 5,
//             'timeout_socket'         => 5,
//         ),
//         'connection_class'         => 'Wrench\Connection',
		 'connection_class'         => 'EveryCook\EveryCookConnection',
//         'connection_options'       => array(
//             'socket_class'           => 'Wrench\Socket\ServerClientSocket',
//             'socket_options'         => array(),
//             'connection_id_secret'   => 'asu5gj656h64Da(0crt8pud%^WAYWW$u76dwb',
//             'connection_id_algo'     => 'sha512'
//         )
     )
));



$server->registerApplication('everycook', new \EveryCook\Application\EveryCookApplication(array(
	'deviceWritePath'=>'/dev/shm/command',
	'deviceReadPath'=>'/dev/shm/status',
	'deviceWriteUrl'=>'/hw/sendcommand.php?command=',
	'deviceReadUrl'=>'/hw/status',
	)));

$server->run();
