#
# MS like keybindings for HTML Editor
#

binding "html-keys"
{
  bind "Home"         { "cursor_move" (left,   all) }
  bind "End"          { "cursor_move" (right,  all) }
  bind "<Ctrl>Home"   { "cursor_move" (up,   all) }
  bind "<Ctrl>End"    { "cursor_move" (down, all) }
}

class "GtkHTML" binding "html-keys"
