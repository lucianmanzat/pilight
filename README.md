New (Experimental) Features
=======
<a class="donate" href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=curlymoo1%40gmail%2ecom&lc=US&item_name=curlymoo&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHostedGuest" target="_blank">
<img alt="PayPal" title="PayPal" border="0" src="https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif" style="max-width:100%;"></a>
<a href="https://flattr.com/submit/auto?user_id=pilight&url=http%3A%2F%2Fwww.pilight.org" target="_blank"><img src="http://api.flattr.com/button/flattr-badge-large.png" alt="Flattr this" title="Flattr this" border="0"></a>

- Modular hardware support.
- All function also works with the pilight config.
- Multiple ID's per device (feature request).
- Fixed relay protocol bug in which in detected the wrong `hw-mode`.
- Allow for protocol specific `send_repeats` and `receive_repeats` setting.
- Allow for protocol specific settings to alter default protocol behavior.
- Fixed bugs of the weather module in the webgui.
- Differentiate between internally communicated settings and external ones.
- Allow (non-blocking) pilight-send call from within process file.
- Added a protocol specific "readonly" setting. This should disable controlling devices from the GUIs.
- Added whitelist setting for socket connections.
- Allow protocol specific pulse lengths
- Default pulse length has been changed from 295 to 294.
- Enabled Impuls receiving.
- Allow for multiple protocols per device.
- Enabled webserver file caching.
- Allow for push messages send from within a protocol.

New config syntax:

```
{
	"living": {
		"name": "Living",
		"bookshelve": {
			"name": "Book Shelve Light",
			"protocol": ["kaku_switch"],
			"id": [{
				"id": 1234,
				"unit": 0
			}],
			"state": "off"
		},
		"main": {
			"name": "Main",
			"protocol": ["kaku_dimmer"],
			"id": [{
				"id": 1234,
				"unit": 1
			}],
			"state": "on",
			"dimlevel", 0
		},
		"television": {
			"name": "Television",
			"protocol": ["relay"],
			"id": [{
				"gpio": 3
			}],
			"state": "off"
		} 
	},
	"bedroom": {
		"name": "Bedroom",
		"main": {
			"name": "Main",
			"protocol": ["elro"],
			"id": [{
				"systemcode": 5678,
				"unitcode": 0,
			}],
			"state": "on"
		}
	},
	"garden": {
		"name": "Garden",
		"weather": {
			"name": "Weather Station",
			"protocol": ["alecto"],
			"id": [{
				"id": 100
			}],
			"humidity": 50,
			"temperature": 1530,
			"battery": 1
		}
	}		
}
```
