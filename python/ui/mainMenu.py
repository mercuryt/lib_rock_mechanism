import arcade
from tkinter import filedialog
class MainMenuView(arcade.View):
    def on_show_view(self):
        arcade.set_background_color(arcade.color.BLACK)
    def on_draw(self):
        self.clear()
        arcade.draw_text("Goblin Pit", 400, 0, arcade.color.GREEN,
                font_size=30, anchor_x = "center")
        # a UIManager to handle the UI.
        self.manager = arcade.gui.UIManager()
        self.manager.enable()
        # Create a vertical BoxGroup to align buttons
        self.v_box = arcade.gui.UIBoxLayout()
        # Create the buttons
        quick_start_button = arcade.gui.UIFlatButton(text="Quick Start", width=200)
        self.v_box.add(quick_start_button.with_space_around(bottom=20))
        quick_start_button.on_click = self.quick_start

        load_button = arcade.gui.UIFlatButton(text="Load", width=200)
        self.v_box.add(load_button.with_space_around(bottom=20))
        load_button.on_click = self.load
        self.manager.draw()
        #TODO: Generate / select world, start in selected world, settings
    def quick_start(self, event):
        global window
        game_view = GameView()
        arcade.get_window().show_view(game_view)
    def load(self, event):
        file_path = filedialog.askopenfilename()
        game_view = GameView(load=filePath)
        arcade.get_window().show_view(game_view)
