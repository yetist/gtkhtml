#
# MS like keybindings for HTML Editor
#

binding "gtkhtml-bindings-ms"
{
  bind "Home"              { "cursor_move" (left,   all) }
  bind "End"               { "cursor_move" (right,  all) }
  bind "<Ctrl>Home"        { "cursor_move" (up,   all) }
  bind "<Ctrl>End"         { "cursor_move" (down, all) }

  bind "<Ctrl>b"           { "command" (toggle-bold) }
  bind "<Ctrl>i"           { "command" (toggle-italic) }
  bind "<Ctrl>u"           { "command" (toggle-underline) }
  bind "<Ctrl>s"           { "command" (toggle-strikeout) }

  bind "<Ctrl>z"           { "command" (undo) }
  bind "<Ctrl>r"           { "command" (redo) }

  bind "<Ctrl><Alt>l"      { "command" (align-left) }
  bind "<Ctrl><Alt>c"      { "command" (align-center) }
  bind "<Ctrl><Alt>r"      { "command" (align-right) }

  bind "Tab"               { "command" (indent-more) }
  bind "<Ctrl><Alt>Tab"    { "command" (indent-less) }

  bind "<Ctrl>0"            { "command" (style-normal) }
  bind "<Ctrl>1"            { "command" (style-header1) }
  bind "<Ctrl>2"            { "command" (style-header2) }
  bind "<Ctrl>3"            { "command" (style-header3) }
  bind "<Ctrl>4"            { "command" (style-header4) }
  bind "<Ctrl>5"            { "command" (style-header5) }
  bind "<Ctrl>6"            { "command" (style-header6) }
  bind "<Ctrl>7"            { "command" (style-pre) }
  bind "<Ctrl>8"            { "command" (style-address) }
  bind "<Ctrl><Alt>1"       { "command" (style-itemdot) }
  bind "<Ctrl><Alt>2"       { "command" (style-itemroman) }
  bind "<Ctrl><Alt>3"       { "command" (style-itemdigit) }

  bind "<Alt>1"             { "command" (size-minus-2) }
  bind "<Alt>2"             { "command" (size-minus-1) }
  bind "<Alt>3"             { "command" (size-plus-0) }
  bind "<Alt>4"             { "command" (size-plus-1) }
  bind "<Alt>5"             { "command" (size-plus-2) }
  bind "<Alt>6"             { "command" (size-plus-3) }
  bind "<Alt>7"             { "command" (size-plus-4) }
}
