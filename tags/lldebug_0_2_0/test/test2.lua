-- test - find
testf = function( num, str, pat, res )
  local s, e, r
  s, e = str:find( pat )
  io.write( "[find "..num.."]\n" )
  io.write( "string = \""..str.."\"\n" )
  io.write( "pattern = \""..pat.."\"\n" )
  if( s ~= nil ) then
    r = str:sub( s, e )
    io.write( "result = \""..r.."\"\n" )
  else
    r = nil
    io.write( "result = nil\n" )
  end
  if( r == res ) then
    io.write( "> OK\n" )
  else
    io.write( "> NG\n" )
  end
end
-- find 1
str = "\\5�!�E�F�}\\}�_�B5\"��x����\"�F�}�_ࢂ��B�Ex����!}"
pat = "��[^�B]*"
testf( 1, str, pat, "��x����\"�F�}�_ࢂ�" )
-- find 2
str = "-�@B�@Y�3�~ZY�I���A�A�@�~�@�~-%%B�����~���I3�Z"
pat = "%w+%W+"
testf( 2, str, pat, "B�@" )
-- find 3
str = "]�A���~��}�D���Iy<��D�F<]�F��~������I�݁�y��A�}�"
pat = "..��.."
testf( 3, str, pat, "�}�D���Iy" )
-- find 4
str = "4�D.aa�F�F[X��.^X��^��[*C�@�H�@�D�H��4*C����"
pat = "([�@-��]+).*%1"
testf( 4, str, pat, "�D.aa�F�F[X��.^X��^��[*C�@�H�@�D" )
-- find 5
str = "�X�G��GBB�G#*�_�_0�~�A��*�|�D���D#�|�~0�AX�G�I�I"
pat = ".*(.).+%1"
testf( 5, str, pat, "�X�G��GBB�G#*�_�_0�~�A��*�|�D���D" )
-- find 6
str = "���G/��./#�|��A���^#ࣂ�����Gz.��E!�^��z�E�A!�|"
pat = "%l%L"
testf( 6, str, pat, "z." )
-- find 7
str = "c�D�@�܏{c|za|{�����{�Bz���@�D�܏\{�B�\��Y��aY��"
pat = "[@Y\\]"
testf( 7, str, pat, "Y" )
-- find 8
str = "�Bab}��@Z�]��ࠃE'}�EX�\'Xb�\�B�Aa�@]Z�A���"
pat = "%U+[�@-�C]+"
testf( 8, str, pat, "b�\�B�A" )
-- find 9
str = "�A�I/�}�A,.�A�I�E�@�E�@�~�A/<��~�C���C�A���A�}.<�,"
pat = "�}[�@�A�B�C�D�E�F�G�H�I�J�K]?[^�@�A�B�~����]*�A"
testf( 9, str, pat, "�}�A,.�A�I�E�@�E�@�~�A/<��~�C���C�A" )
-- find 10
str = "�C5.���D�C�.�H����^���~�~5=�~���D�^(=ށ~�H����(�"
pat = "%f[�~].%d.."
testf( 10, str, pat, nil )
-- find 11
str = "`�`0�|�{�A�{�)~�A@�|~�F0;�F}3)3��@���J�J;}"
pat = "%d%D*%d"
testf( 11, str, pat, "0�|�{�A�{�)~�A@�|~�F0" )
-- find 12
str = "�~|�@|5���@����~�܁�0cc�_��0�K�܏_���K\\�}�}��\\�5"
pat = "%d%S-%d"
testf( 12, str, pat, "5���@����~�܁�0" )
-- find 13
str = "+/�C~4~)(5��(*�����K�*�F����K�H5+)4(�F/�H�C"
pat = "%b()"
testf( 13, str, pat, "(*�����K�*�F����K�H5+)" )
-- find 14
str = "b߁���߂�}�B~}b`.�~�@��.~������+��+�����B��`��"
pat = "%b����[^%d%l%w%s]"
testf( 14, str, pat, "����߂�}�B~}b`.�~�@��.~������+" )
-- find 15
str = "�G�����B�}��8���B�@8�@��.��G������~�B�܁B�~���}.ࢂ��"
pat = "%..-$"
testf( 15, str, pat, ".��G������~�B�܁B�~���}.ࢂ��" )
-- find 16
str = "��@!!;{��^)Z;���@xZ�G^ޟ�x1�{�@�G){�{1�@"
pat = "^%a%A"
testf( 16, str, pat, nil )
-- find 17
str = "�[ࠡ�A/#�[6/���\,#=,�{X���FX�6�{���F�\=�A�"
pat = "\\"
testf( 17, str, pat, nil )
-- find 18
str = "�}��#\\5�B�}��;XX�\\�B���A�B�K5�[����;ܟB���[�A#�K"
pat = "\\.-\\"
testf( 18, str, pat, "\\5�B�}��;XX�\\" )
-- find 19
str = "���=���~�|<�@�Eࢠߏ~{߃C���@��|�_�C�@��{�E�_=<�@"
pat = "^..............................$"
testf( 19, str, pat, "���=���~�|<�@�Eࢠߏ~{߃C���@��|�_�C�@��{�E�_=<�@" )
-- find 20
str = "�|�A��2��#_�]�F#�ܟB�|��B�G2��''���_�G�]ࠟB�AB�F"
pat = "�G[^a-eA-E%%]-[a-eA-E%%]?�"
testf( 20, str, pat, "�G2��''���" )
-- test - gmatch
testgf = function( num, str, pat, res )
  local p, r
  r = ""
  io.write( "[gmatch "..num.."]\n" )
  io.write( "string = \""..str.."\"\n" )
  io.write( "pattern = \""..pat.."\"\n" )
  io.write( "result = \"" )
  for p in str:gmatch( pat ) do
    if( r == "" ) then
      r = r..p
    else
      r = r..","..p
    end
  end
  io.write( r.."\"\n" )
  if( r == res ) then
    io.write( "> OK\n" )
  else
    io.write( "> NG\n" )
  end
end
-- gmatch 1
str = "�B�{�B����X�I�B)^'�H{�A'���H�Ay�)/���By�IX^/"
pat = ".[{/^']"
testgf( 1, str, pat, "�{,)^,�H{,�A',)/,X^" )
-- gmatch 2
str = "��I�~��2;��=�C_��I��|�F!�D�~=2����_|�D��!�F�C;"
pat = "..[%d;=!]?"
testgf( 2, str, pat, "��I,�~��2,;��=,�C_,��I,��|,�F!,�D�~=,2��,��_,|�D,��!,�F�C;" )
-- gmatch 3
str = "��!�_�[,,�G�E5��?5=?@�Gc���D�D=!���EYcY@�[�_"
pat = "[^,?]+"
testgf( 3, str, pat, "��!�_�[,�G�E5��,5=,@�Gc���D�D=!���EYcY@�[�_" )
-- gmatch 4
str = "*���A�+���~�H�~�H�A�^�����B�ݟA�B�ݟA��^��+����*�A"
pat = "(.+)%1"
testgf( 4, str, pat, "�~�H,��,�B�ݟA" )
-- gmatch 5
str = ";$݂�$���{�@.%��@�K�{���@.�����@�@�zz;�@%��K��"
pat = ".((.).-)%2"
testgf( 5, str, pat, "$݂�,�{�@.%��@�K,�@.�����@,z" )
-- gmatch 6
str = "��55�@'B�\"/�~�A�I�ݟ~܏@7��:B�A7\"':�I/���A�A"
pat = "%W+"
testgf( 6, str, pat, "��,�@',�\"/�~�A�I�ݟ~܏@,��:,�A,\"':�I/���A�A" )
-- gmatch 7
str = "?�B?@�C��`[C�@C[6�E`�C���B܂�<���E6,����,<"
pat = "[^%z\001-\255]+"
testgf( 7, str, pat, "�B,�C��,�E,�C���B,��,���E,����" )
-- gmatch 8
str = "�E���]^܁~\"#��Y�B#�}\"�GY�^]8�}C�G8�E�~�B�C"
pat = "[%z\001-\255]+"
testgf( 8, str, pat, "�,]^�,\"#,Y,#,\",Y�^]8,C,8,�C" )
-- gmatch 9
str = ".��x>Z�Dz���8z�Z/�ޏ[�D/8��@).)@x>�[[["
pat = "%w%W%W-%w"
testgf( 9, str, pat, "x>Z,z���8,z�Z,8��@).)@x" )
-- gmatch 10
str = "Yb�F�H�_�}�E8b8a�H�F�IY���߃E���_a��I'�@'�ߏ}݁@"
pat = "%f[�I���H�_�}].[^�I���H�_�}]+"
testgf( 10, str, pat, "�H�F,�IY��,�߃E��,�_a�,�I'�@'" )
-- test - gsub
testgs = function( num, str, pat, repl, res )
  local r
  r = ""
  io.write( "[gsub "..num.."]\n" )
  io.write( "string = \""..str.."\"\n" )
  io.write( "pattern = \""..pat.."\"\n" )
  io.write( "rstring = \""..tostring( repl ).."\"\n" )
  io.write( "result = \"" )
  r = str:gsub( pat, repl )
  io.write( r.."\"\n" )
  if( r == res ) then
    io.write( "> OK\n" )
  else
    io.write( "> NG\n" )
  end
end
-- gsub 1
testgs( 1, "��������������������", "([����])([����])", "%2 %1", "���� ������������ ����" )
-- gsub 2
testgs( 2, "CBA�~ABC�����@@@@@�[AAA�\BB�]C�^C�_BB�{AAA�|@@@@�}�~�@AC�ACA�BBA", "[@ABC]+", "", "�~�����@�[�\�]�^�_�{�|�}�~�@�A�B" )
-- gsub 3
repl = {}
repl["��"] = "�A"; repl["��"] = "�C"; repl["��"] = "�E"; repl["��"] = "�E"; repl["��"] = "�I";
testgs( 3, "����������������������", ".", repl, "���A���C�C�C���E���I�E" )
-- gsub 4
repl = function( word1, word2 )
  local r
  r = ""
  if( word1 ~= "" ) then
    r = "["..word1.."]"
  end
  if( word2 ~= "" ) then
    r = r.."["..word2.."]"
  end
  return r
end
testgs( 4, "���� ���R �ӂ� �t�j �Ӄ��t��", "([��-��]*)%S?(%S?)", repl, "[����] [�R] [�ӂ�] [�j] [��][�t][��]" )
-- test - format
testfm = function( num, frm, str, res )
  local r
  io.write( "[format "..num.."]\n" )
  io.write( "format = '"..frm.."'\n" )
  io.write( "string = '"..str.."'\n" )
  io.write( "result = '" )
  r = string.format( frm, str )
  -- r = string.format('%q', 'a string with "quotes" and \n new line')
  io.write( r.."'\n" )
  if( r == res ) then
    io.write( "> OK\n" )
  else
    io.write( "> NG\n" )
  end
end
-- format 1
testfm( 1, '%q', '�\�]\\�\\t\\�]%%$"��"DON', '"�\�]\\\\�\\t\\\\�]%%$\\"��\\"DON"' )
-- format 2
testfm( 2, 'te%qst', '\000', 'te"\\000"st' )
-- test - match
testm = function( num, str, pat, res1, res2 )
  local r1, r2
  r1, r2 = str:match( pat )
  io.write( "[match "..num.."]\n" )
  io.write( "string = \""..str.."\"\n" )
  io.write( "pattern = \""..pat.."\"\n" )
  io.write( "result1 = \""..tostring( r1 ).."\"\n" )
  io.write( "result2 = \""..tostring( r2 ).."\"\n" )
  if( r1 == res1 and r2 == res2 ) then
    io.write( "> OK\n" )
  else
    io.write( "> NG\n" )
  end
end
-- match 1
testm( 1, "[[http://google.com]]", "(https?)://([%w./]*)", "http", "google.com" )
-- match 2
testm( 2, "�L�q�L�q�����Ղ肽�炱", "(��?)[^��]+(��?)", "", "��" )
-- match 3
testm( 3, string.sub( "ABC������EFG���~��", 1, 17 ), "[^\129-\159\224-\252%z]+", "ABC������EFG���~", nil )
-- match 4
testm( 4, "A�AA�A�AA��A�ABC��A��A��AB�AA��A�AA��A��A��CD�AA�A", "(.*)(%w%w.*)%w%w", "A�AA�A�AA��A�ABC��A��A��", "AB�AA��A�AA��A��A��" )
