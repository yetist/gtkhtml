#
# MS like keybindings for HTML Editor
#

binding "html-keys"
{
  bind "Home"              { "cursor_move" (left,   all) }
  bind "End"               { "cursor_move" (right,  all) }
  bind "<Ctrl>Home"        { "cursor_move" (up,   all) }
  bind "<Ctrl>End"         { "cursor_move" (down, all) }

  bind "<Ctrl>b"           { "command" (toggle-bold) }
  bind "<Ctrl>i"           { "command" (toggle-italic) }
  bind "<Ctrl>u"           { "command" (toggle-underline) }
  bind "<Ctrl>s"           { "command" (toggle-strikeout) }

  bind "<Ctrl>l"           { "command" (align-left) }
  bind "<Ctrl>c"           { "command" (align-center) }
  bind "<Ctrl>r"           { "command" (align-right) }

  bind "Tab"               { "command" (indent-more) }
  bind "<Ctrl><Alt>Tab"    { "command" (indent-less) }
}

class "GtkHTML" binding "html-keys"
