# LiteNPC

Simple plugin that provides API for creating Custom Player NPCs

Example:
```js
import LiteNPC from "plugins/LiteNPC/API.js"
LiteNPC.plugin("my_plugin_name");

LiteNPC.create("name", new FloatPos(0, 0, 0, 0), new DirectionAngle(0, 180), "skin_name",
	pl => {
		pl.tell("Hello!")
	});
```
Save your current skin by command: `/saveskin skin_name`

