import bound
import ui
import arcade

WIDTH = 800
HEIGHT = 600

if __name__ == "__main__":
    global window
    window = arcade.Window(WIDTH, HEIGHT, "Goblin Pit")
    menu_view = ui.MainMenuView()
    window.show_view(menu_view)
    arcade.run()
