import arcade
class GameView(arcade.View):
    def __init__(self, load = False):
        global bound
        self.bound = bound;
        self.terrain_sprite_list = arcade.SpriteList()
        self.actors_sprite_list = actors.SpriteList()
        if(!load):
            load = defaultScenarioPath
    def on_show_view(self):
        # do load
        pass
    def on_draw(self):
        self.clear()
        self.terrain_sprite_list.draw()
        self.actors_sprite_list.draw()
