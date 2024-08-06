# LiteNPC

Simple plugin that provides API for creating Custom Player NPCs

Example:
```js
import LiteNPC from "plugins/LiteNPC_.js"

LiteNPC.create("name", new FloatPos(2, 0, 0, 0), new DirectionAngle(0, 180), "skins/skin.png",
	pl => {
		pl.tell("Hello!")
	});
```

