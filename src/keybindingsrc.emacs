#
# emacs like keybindings for HTML Editor
#

binding "html-keys"
{
  bind "Home"               { "cursor_move" (up,   all) }
  bind "End"                { "cursor_move" (down, all) }
  bind "<Alt>less"          { "cursor_move" (up,   all) }
  bind "<Alt>greater"       { "cursor_move" (down, all) }
  bind "<Ctrl>b"            { "cursor_move" (left,  one) }
  bind "<Ctrl>f"            { "cursor_move" (right, one) }
  bind "<Ctrl>Left"         { "cursor_move" (left,  word) }
  bind "<Ctrl>Right"        { "cursor_move" (right, word) }
  bind "<Alt>b"             { "cursor_move" (left,  word) }
  bind "<Alt>f"             { "cursor_move" (right, word) }
  bind "<Ctrl>p"            { "cursor_move" (up,    one) }
  bind "<Ctrl>n"            { "cursor_move" (down,  one) }

  bind "<Alt>v"             { "cursor_move" (up,    page) }
  bind "<Ctrl>v"            { "cursor_move" (down,  page) }

  bind "<Ctrl>d"            { "command" (delete) }
  bind "<Ctrl>g"            { "command" (disable-selection) }
  bind "<Ctrl>m"            { "command" (insert-paragraph) }
  bind "<Ctrl>j"            { "command" (insert-paragraph) }
  bind "<Ctrl>w"            { "command" (cut) }
  bind "<Alt>w"             { "command" (copy) }
  bind "<Ctrl>y"            { "command" (paste) }
  bind "<Ctrl>space"        { "command" (set-mark) }

  bind "<Ctrl><Alt>b"       { "command" (toggle-bold) }
  bind "<Ctrl><Alt>i"       { "command" (toggle-italic) }
  bind "<Ctrl><Alt>u"       { "command" (toggle-underline) }
  bind "<Ctrl><Alt>s"       { "command" (toggle-strikeout) }

  bind "<Ctrl><Alt>l"       { "command" (align-left) }
  bind "<Ctrl><Alt>c"       { "command" (align-center) }
  bind "<Ctrl><Alt>r"       { "command" (align-right) }

  bind "Tab"                { "command" (indent-more) }
  bind "<Ctrl><Alt>Tab"     { "command" (indent-less) }
}

class "GtkHTML" binding "html-keys"
