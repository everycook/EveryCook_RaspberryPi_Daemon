<?php

namespace EveryCook\Application;

use Wrench\Application\Application;

/**
 * Example application for Wrench: echo server
 */
class EveryCookApplication extends Application
{
    protected $_clients = array();
    protected $_lastTimestamp = null;
	protected $_firmware_client = null;
	protected $_server = null;
	
	private function readActionFromFirmware($info, $recipeNr){
		$dest = $info->cookWith[$recipeNr];
		$inhalt = '';
		if ($dest[0] == self::COOK_WITH_LOCAL){
			$fw = fopen(Yii::app()->params['deviceReadPath'], "r");
			if ($fw !== false){
				while (!feof($fw)) {
					$inhalt .= fread($fw, 128);
				}
				fclose($fw);
			} else {
				//TODO: error on read status
				$inhalt = 'ERROR: $errstr ($errno)';
			}
		} else if ($dest[0] == self::COOK_WITH_IP){
			require_once("remotefileinfo.php");
			$inhalt=remote_file('http://'.$dest[2].Yii::app()->params['deviceReadUrl']);
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
	
	public function getUpdateState($recipeNr){
		$info = null;
		if (apc_exists('cookingInfo')){
			$info = apc_fetch('cookingInfo');
		}
		if (!$info){
			return '{"error":"Current cooking information not found!"}';
		}
		//$info = Yii::app()->session['cookingInfo'];
		
		if (isset($info->cookWith[$recipeNr]) && $info->cookWith[$recipeNr][0]!=self::COOK_WITH_OTHER){
			$state = $this->readActionFromFirmware($info, $recipeNr);
			if (is_string($state) && strpos($state,"ERROR: ") !== false){
				return '{"error":"' . substr($state, 7) . '"}';
			}
			
			$mealStep = $info->steps[$recipeNr];
			$mealStep->HWValues = $state;
			
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
			
			$mealStep->percent = $percent;
			$mealStep->nextStepIn = $restTime;
			
			if (apc_store('cookingInfo', $info)){
				//error storig attribute...
			}
			
			$additional.=', T0:' . $state->T0;
			$additional.=', P0:' . $state->P0;
			
			return '{percent:' . $percent . ', restTime:' . $restTime .$additional . ', startTime:'.$_GET['startTime'] . '}';
			
			//{"T0":100,"P0":0,"M0RPM":0,"M0ON":0,"M0OFF":0,"W0":0,"STIME":30,"SMODE":10,"SID":0}
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
			if ($this->_firmware_client == null){
				//TODO: if no server connected, read status from filesystem...
			}
			
			// limit updates to once per second
			if(time() > $this->_lastTimestamp) {
				$this->_lastTimestamp = time();
				//$this->_sendAll(date('d-m-Y H:i:s'));;
				//$this->_sendAll($this->getUpdateState(0));
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
				$this->_sendAll($payload);
			} else {
				$client->log("send all, from other client?: " . $payload);
				//TODO what to do when there are a messge from other server?
				$this->_sendAll($payload);
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