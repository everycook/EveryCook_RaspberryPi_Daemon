<?php 
	$fp = fopen("writefile.txt", "w");
	fputs($fp, "BUZZER	".$_POST["buzzer"].";\n");
	fputs($fp, "BZRPWM	".$_POST["buzzernum"].";\n");
	fputs($fp, "KEY1	".$_POST["key1"].";\n");
	fputs($fp, "KEY2	".$_POST["key2"].";\n");
	fputs($fp, "KEY3	".$_POST["key3"].";\n");
	fputs($fp, "KEY4	".$_POST["key4"].";\n");
	fputs($fp, "PWM0	".$_POST["pwm0"].";\n");
	fputs($fp, "PWM1	".$_POST["pwm1"].";\n");
	fputs($fp, "PWM2	".$_POST["pwm2"].";\n");
	fputs($fp, "PWM3	".$_POST["pwm3"].";\n");
	fputs($fp, "PWM4	".$_POST["pwm4"].";\n");
	fputs($fp, "PWM5	".$_POST["pwm5"].";\n");
	fputs($fp, "PWM6	".$_POST["pwm6"].";\n");
	fputs($fp, "PWM7	".$_POST["pwm7"].";\n");
	fputs($fp, "PWM8	".$_POST["pwm8"].";\n");
	fputs($fp, "PWM9	".$_POST["pwm9"].";\n");
	fputs($fp, "PWM10	".$_POST["pwm10"].";\n");
	fputs($fp, "PWM11	".$_POST["pwm11"].";\n");
	fputs($fp, "PWM12	".$_POST["pwm12"].";\n");
	fputs($fp, "PWM13	".$_POST["pwm13"].";\n");
	fputs($fp, "PWM14	".$_POST["pwm14"].";\n");
	fputs($fp, "PWM15	".$_POST["pwm15"].";\n");
	fputs($fp, ".\n");
	fclose($fp);
?>
