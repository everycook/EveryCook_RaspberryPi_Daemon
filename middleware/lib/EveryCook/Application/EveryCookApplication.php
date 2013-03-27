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

namespace EveryCook\Application;

use Wrench\Application\Application;

class EveryCookApplication extends Application
{

	const STANDBY=0;
	const CUT=1;
	const MOTOR=1;
	const SCALE=2;
	const HEADUP=10;
	const COOK=11;
	const COOLDOWN=12;
	const PRESSUP=20;
	const PRESSHOLD=21;
	const PRESSDOWN=22;
	const PRESSVENT=23;
	
	const HOT=30;
	const PRESSURIZED=31;
	const COLD=32;
	const PRESSURELESS=33;
	const WEIGHT_REACHED=34;
	const COOK_TIMEEND=35;
	const RECIPE_END=39;
	
	const INPUT_ERROR=40;
	const EMERGANCY_SHUTDOWN=41;
	const MOTOR_OVERLOAD=42;
	
	const COMMUNICATION_ERROR=53;
	
	
	const COOK_WITH_OTHER = 0;
	const COOK_WITH_LOCAL = 1;
	const COOK_WITH_IP = 2;
	const COOK_WITH_IP_DEFAULT = '10.0.0.1';
	const COOK_WITH_EVERYCOOK_COI = 1;
	
	
	
    protected $_clients = array();
    protected $_lastTimestamp = null;
	protected $_firmware_client = null;
	protected $_server = null;
	
	protected $_deviceReadPath = null;
	protected $_deviceReadUrl = null;
	protected $_options = null;
	protected $debug = false;
	
	protected $_memcached = null;
	
	protected function getMemcached(){
		if ($this->_memcached == null){
			$this->_memcached = new \Memcached();
			$this->_memcached->addServer($this->_options['memcachedHost'], $this->_options['memcachedPort']);
		}
		return $this->_memcached;
	}
	
	protected function getFromCache($name){
		//return Yii::app()->session[$name];
		//return apc_fetch($name);
		$memcached = $this->getMemcached();
		return $memcached->get($name."_stdobj");
	}
	
	protected function saveToCache($name, $value){
		//Yii::app()->session['cookingInfo'] = $info;
		//if(apc_store($name, $value)){
		$memcached = $this->getMemcached();
		if($memcached->set($name."_stdobj", $value)){
		/*
			echo "save successfull...";
		} else {
			echo "save failed...";
		*/
		}
	}
	
	public function __construct($options){
        $this->_options = array_merge(array(
			'deviceWritePath'=>'/dev/shm/command',
			'deviceReadPath'=>'/dev/shm/status',
			'deviceWriteUrl'=>'/hw/sendcommand.php?command=',
			'deviceReadUrl'=>'/hw/status',
			'memcachedHost'=>'localhost',
			'memcachedPort'=>11211,
        ), $options);
	}
	
	private function readFromFirmware($info, $recipeNr){
		$dest = $info->cookWith[$recipeNr];
		$inhalt = '';
		if ($dest[0] == self::COOK_WITH_LOCAL){
			if(file_exists($this->_options['deviceReadPath'])){
				$fw = fopen($this->_options['deviceReadPath'], "r");
				if ($fw !== false){
					while (!feof($fw)) {
						$inhalt .= fread($fw, 128);
					}
					fclose($fw);
				} else {
					//TODO: error on read status
					$inhalt = 'ERROR: $errstr ($errno)';
				}
			} else {
				$inhalt = 'ERROR: file ' . $this->_options['deviceReadPath'] . ' does not exist';
			}
		} else if ($dest[0] == self::COOK_WITH_IP){
			require_once("remotefileinfo.php");
			$inhalt=remote_file('http://'.$dest[2].$this->_options['deviceReadUrl']);
		}
		
		//$inhalt='{"T0":100,"P0":0,"M0RPM":0,"M0ON":0,"M0OFF":0,"W0":0,"STIME":5,"SMODE":1,"SID":0}';
		if (strpos($inhalt,"ERROR: ") !== false){
			$inhalt = str_replace("\r\n","", $inhalt);
			$inhalt = str_replace("<br />","", $inhalt);
			$inhalt = trim($inhalt);
			return $inhalt;
		} else {
			$jsonValue=json_decode($inhalt);
			return $jsonValue;
		}
	}
	
	public function getUpdateState($info, $recipeNr, $state){
		echo "getUpdateState\r\n";
		if (isset($info->cookWith[$recipeNr]) && isset($info->cookWith[$recipeNr][0]) && $info->cookWith[$recipeNr][0]!=self::COOK_WITH_OTHER){
			$mealStep = $info->steps[$recipeNr];
			$mealStep->HWValues = $state;
			print_r($state);
			$this->saveToCache('HWValues', $state);
			
			if ($info->steps[$recipeNr]->endReached){
				$additional=', T0:' . $state->T0;
				$additional.=', P0:' . $state->P0;
				$mealStep->currentTemp = $state->T0;
				$mealStep->currentPress = $state->P0;
				return '{percent:1, restTime:0' .$additional . ', startTime:'.$_GET['startTime'] . '}';
			}
			
			$recipe = $info->course->couToRecs[$recipeNr]->recipe;
			$step = $info->recipeSteps[$recipeNr][$info->stepNumbers[$recipeNr]];
			$executetTime = time() - $info->stepStartTime[$recipeNr];
			
			$currentTime = time();
			$stepStartTime = $info->stepStartTime[$recipeNr];
			$mealStep->nextStepIn = $stepStartTime - $currentTime + $mealStep->nextStepTotal;
			$mealStep->inTime = $stepStartTime + $mealStep->nextStepTotal < $currentTime;
			$restTime = $mealStep->nextStepIn;
			
			printf("executetTime:%s, currentTime:%s, stepStartTime:%s, restTime:%s", $executetTime, $currentTime, $stepStartTime, $restTime);
			
			//$restTime = $state->STIME;
			$additional='';
			if ($state->SMODE==self::STANDBY || $state->SMODE==self::CUT || $state->SMODE==self::MOTOR || $state->SMODE==self::COOK || $state->SMODE==self::PRESSHOLD || $state->SMODE==self::COOK_TIMEEND || $state->SMODE==self::RECIPE_END){
				//$percent = 1 - ($state->STIME / $step['STE_STEP_DURATION']);
				if (isset($mealStep->nextStepTotal) && $mealStep->nextStepTotal >0){
					$percent = 1 - ($restTime / $mealStep->nextStepTotal);
				} else {
					$percent = 1;
				}
			} else if ($state->SMODE==self::SCALE || $state->SMODE==self::WEIGHT_REACHED){
				$weight = floor($state->W0);
				if ($step['STE_GRAMS'] != 0){
					$percent = $weight / $step['STE_GRAMS'];
				} else {
					$percent = 1;
				}
				$additional=', W0:' . $state->W0;
				if ($percent>0.05){ //>5%
					//$restTime = round(($executetTime / $percent) - $executetTime);
					$text = '<span class=\"ingredient\">' . $step['ING_NAME_' . Yii::app()->session['lang']] . '</span> <span class=\"amount\">' . $step['STE_GRAMS'] . 'g' . '</span>: ' . round($percent*100) . '% / ' . round($state->W0) . 'g';
					if ($percent>1.05){
						$text = '<span class=\"toMuch\">' . $text . '</span>';
					}
					$additional .= ', text: "' . $text . '"';
				}
			} else if ($state->SMODE==self::HEADUP || $state->SMODE==self::HOT){
				$percent = $state->T0 / $step['STE_CELSIUS'];
				if ($percent>0.05){ //>5%
					$restTime = round(($executetTime / $percent) - $executetTime);
				}
			} else if ($state->SMODE==self::COOLDOWN || $state->SMODE==self::COLD){
				$percent = $step['STE_CELSIUS'] / $state->T0; //TODO: correct?
				if ($percent>0.05){ //>5%
					$restTime = round(($executetTime / $percent) - $executetTime);
				}
			} else if ($state->SMODE==self::PRESSUP || $state->SMODE==self::PRESSURIZED){
				$percent = $state->P0 / $step['STE_KPA'];
				if ($percent>0.05){ //>5%
					$restTime = round(($executetTime / $percent) - $executetTime);
				}
			} else if ($state->SMODE==self::PRESSDOWN  || $state->SMODE==self::PRESSVENT ||$state->SMODE==self::PRESSURELESS){
				$percent = $step['STE_KPA'] / $state->P0; //TODO: correct?
				if ($percent>0.05){ //>5%
					$restTime = round(($executetTime / $percent) - $executetTime);
				}
			} else if ($state->SMODE==self::INPUT_ERROR){
				return '{"error":"' . 'Input Error' . '"}';
			} else if ($state->SMODE==self::EMERGANCY_SHUTDOWN){
				return '{"error":"' . 'Emergency shutdown!'  . '"}';
			} else if ($state->SMODE==self::MOTOR_OVERLOAD){
				return '{"error":"' . 'Motor overload'  . '"}';
			} else {
				return '{"error":"' . 'Unknown EveryCook State/Mode:' . $state->SMODE . '"}';
			}
			$percent = round($percent, 2);
			if ($state->SMODE >= 30 && $state->SMODE <= 39){
				//Auto Next:
				if ($state->SMODE == self::WEIGHT_REACHED){
					if ($percent>=0.95 && $percent<=1.05){
						//Wait 5 Sec with no change
						if ($mealStep->percent == $percent && $mealStep->weightReachedTime != 0){
							if ($currentTime - $mealStep->weightReachedTime >=5){
								$additional.=', gotoNext: true';
							}
						} else {
							$mealStep->weightReachedTime = $currentTime;
						}
					} else {
						$mealStep->weightReachedTime = 0;
					}
				} else {
					$additional.=', gotoNext: true';
				}
			}
			
			
			printf("additional:%s", $additional);
			
			$mealStep->percent = $percent;
			$mealStep->nextStepIn = $restTime;
			
			$this->saveToCache('cookingInfo', $info);
			
			$additional.=', T0:' . $state->T0;
			$additional.=', P0:' . $state->P0;
			
			return '{percent:' . $percent . ', restTime:' . $restTime .$additional . '}'; //', startTime:'.$_GET['startTime']
			
			//{"T0":100,"P0":0,"M0RPM":0,"M0ON":0,"M0OFF":0,"W0":0,"STIME":30,"SMODE":10,"SID":0}
		}
	}
	
	private function sendState($firmwareState){
		$recipeNr = 0;
		$info = $this->getFromCache('cookingInfo');
		if (!$info){
			$this->_sendAll('{"error":"Current cooking information not found!"}');
			return;
		}
		$firmwareState = json_decode($firmwareState);
		
		$state = $this->getUpdateState($info, $recipeNr, $firmwareState);
		$this->_sendAll($state);
	}
	
	private function getAndSendState(){
		$recipeNr = 0;
		$info = $this->getFromCache('cookingInfo');
		if (!$info){
			$this->_sendAll('{"error":"Current cooking information not found!"}');
			return;
		}
		//$info = Yii::app()->session['cookingInfo'];
		
		$firmwareState = $this->readFromFirmware($info, $recipeNr);
		
		if (is_string($firmwareState) && strpos($firmwareState,"ERROR: ") !== false){
			$this->_sendAll('{"error":"' . substr($firmwareState, 7) . '"}');
			return;
		}
		
		$state = $this->getUpdateState($info, $recipeNr, $firmwareState);
		$this->_sendAll($state);
	}
	
	private function sendCommand($command){
		$recipeNr = 0;
		$info = $this->getFromCache('cookingInfo');
		if (!$info){
			$this->_sendAll('{"error":"Current cooking information not found!"}');
			return;
		}
		
		$this->sendActionToFirmware($info, $recipeNr, $command);
	}
	
	private function sendActionToFirmware($info, $recipeNr, $command){
		try{
			if (isset($info->cookWith[$recipeNr]) && count($info->cookWith[$recipeNr])>0 && $info->cookWith[$recipeNr][0]!=self::COOK_WITH_OTHER){
				if (isset($info->recipeSteps[$recipeNr][$info->stepNumbers[$recipeNr]])){
					if ($this->debug){
						echo '<script type="text/javascript"> if(console && console.log){ console.log(\'sendActionToFirmware, command: '.$command.'\')}</script>';
					}
					
					$dest = $info->cookWith[$recipeNr];
					//TODO: remove return
					return;
					if ($dest[0] == self::COOK_WITH_LOCAL){
						$fw = fopen(Yii::app()->params['deviceWritePath'], "w");
						if (fwrite($fw, $command)) {
						} else {
							//TODO error an send command...
						}
						fclose($fw);
					} else if ($dest[0] == self::COOK_WITH_IP){
						require_once("remotefileinfo.php");
						$inhalt=remote_fileheader('http://'.$dest[2].Yii::app()->params['deviceWriteUrl'].$command); //remote_file
						if (is_string($inhalt) && strpos($inhalt, 'ERROR: ') !== false){
							//TODO error an send command...
						}
					}
				} else {
					if ($this->debug){
						echo 'console.log(\'sendActionToFirmware, $info->recipeSteps[$recipeNr][$info->stepNumbers[$recipeNr]] not set\');';
						//echo 'console.log(\'sendActionToFirmware, count($info->course->couToRecs['.$recipeNr.']->recipe->steps) = '.count($info->course->couToRecs[$recipeNr]->recipe->steps).'\');';
						//echo 'console.log(\'sendActionToFirmware, $info->stepNumbers['.$recipeNr.'] = '.$info->stepNumbers[$recipeNr].'\');';
					}
				}
			} else {
				if ($this->debug){
					echo 'console.log(\'sendActionToFirmware, isset($info->cookWith['.$recipeNr.'])='.isset($info->cookWith[$recipeNr]).'\');';
					if (isset($info->cookWith[$recipeNr])){
						echo 'console.log(\'sendActionToFirmware, count($info->cookWith['.$recipeNr.'])='.count($info->cookWith[$recipeNr]).' \');';
						if (count($info->cookWith[$recipeNr])>0){
							echo 'console.log(\'sendActionToFirmware, $info->cookWith['.$recipeNr.'][0]= '.$info->cookWith[$recipeNr][0].'\');';
						}
					}
				}
			}
		} catch(Exception $e) {
			if ($this->debug) echo 'Exception occured in sendActionToFirmware for recipeIndex ' . $recipeNr . ', Exeption was: ' . $e;
		}
	}
	
    /**
     * @see Wrench\Application.Application::onUpdate()
     */
    public function onUpdate(){
		try {
			if (count($this->_clients) < 1) {
				return false;
			}
			
			// limit updates to once per second
			if(time() > $this->_lastTimestamp) {
				$this->_lastTimestamp = time();
				if ($this->_firmware_client == null){
					//TODO: if no server connected, read status from filesystem...
					//$this->_sendAll(date('d-m-Y H:i:s'));
					$this->getAndSendState();
				}
			}
		} catch(Exception $e) {
			if ($this->_server != null){
				$this->_server->log('Exception in onUpdate(Application: ' . $this . '): ' . $e, 'error');
			}
		}
    }
	
    /**
     * @see Wrench\Application.Application::onConnect()
     */
    public function onServerConnect($client){
		if ($this->_firmware_client != null && $this->_firmware_client != $client){
			$this->_firmware_client->close();
		}
        $this->_firmware_client = $client;
		$client->log("server connected: " . $client->getId());
    }
	
    public function onServerConnectData($payload, $client){
		try {
			if ($client === $this->_firmware_client){
				$client->log("send all: " . $payload);
				//$this->_sendAll($payload);
				$this->sendState($payload->getPayload());
			} else {
				//TODO what to do when there are a messge from other server?
				$client->log("data from other server???: " . $payload);
				$this->sendState($payload->getPayload());
			}
		} catch(Exception $e) {
			if ($client != null){
				$client->log('Exception in onServerConnectData: ' . $e, 'error');
			}
		}
    }
	
    /**
     * @see Wrench\Application.Application::onData()
     */
    public function onData($payload, $client){
		try {
			if ($this->_firmware_client != null){
				$client->log("data from client, send it to firmware: " . $payload);
				$this->_firmware_client->send($payload);
			} else {
				$client->log("data from client, but no firmware connected: " . $payload);
				$this->sendCommand($payload);
			}
		} catch(Exception $e) {
			if ($client != null){
				$client->log('Exception in onData: ' . $e, 'error');
			}
		}
    }
	
    /**
     * @see Wrench\Application.Application::onBinaryData()
     */
    public function onBinaryData($payload, $client){
        //$client->send($payload);
    }
	
    /**
     * @see Wrench\Application.Application::onRegister()
     */
    public function onRegister($server){
		$this->_server = $server;
    }
	
    /**
     * @see Wrench\Application.Application::onConnect()
     */
    public function onConnect($client){
        $id = $client->getId();
        $this->_clients[$id] = $client;
    }

    /**
     * @see Wrench\Application.Application::onDisconnect()
     */
    public function onDisconnect($client){
        $id = $client->getId();
        unset($this->_clients[$id]);
    }
	
    private function _sendAll($data){
        if (count($this->_clients) < 1) {
            return false;
        }

        foreach ($this->_clients as $sendto) {
			try {
				$sendto->send($data);
			} catch(Exception $e) {
				if ($sendto != null){
					$sendto->log('Exception on send data: ' . $e, 'error');
				}
			}
        }
    }
}