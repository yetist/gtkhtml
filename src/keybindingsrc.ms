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

  bind "<Ctrl>l"           { "command" (align-left) }
  bind "<Ctrl>c"           { "command" (align-center) }
  bind "<Ctrl>r"           { "command" (align-right) }

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
}
