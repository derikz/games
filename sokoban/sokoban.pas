
program sokoban;

{ 2.0     1990-09  converted from Turbo Pascal to Fitted Modula-2 }
{ 2.1     2010-04  converted to GNU Modula-2 }
{ 2.1.1   2015-12  english variable names and comments, emacs indentation }
{ 3.0     2015-12  converted to Free Pascal }

{  Copyright (c) 1990,2010,2015 Derik van Zuetphen <dz@426.ch> }
{  All rights reserved. }

{  Redistribution and use in source and binary forms, with or without }
{  modification, are permitted provided that the following conditions }
{  are met: }

{  1. Redistributions of source code must retain the above copyright }
{     notice, this list of conditions and the following disclaimer. }
{  2. Redistributions in binary form must reproduce the above copyright }
{     notice, this list of conditions and the following disclaimer in the }
{     documentation and/or other materials provided with the distribution. }

{  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, }
{  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY }
{  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL }
{  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, }
{  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, }
{  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; }
{  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, }
{  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR }
{  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF }
{  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. }

{$MODE OBJFPC}

uses
  crt,
  classes,
  strutils,
  sysutils;

const
    OFFSET   = 19;      { corresponds to a size of 19 columns  }
    MAXPOS   = 303;     { and (303+1)/19 = 16 rows.            }
    MAXLEVEL = 50;
    esc      = #27;
    del      = #8;

type
    ActionType = (move_left, move_right, move_up, move_down, undo_move, open_menu);
    ActionSet = set of ActionType;
    ExtendedChar = (no_key, up_key, down_key, right_key, left_key);
    CellType = (empty, box, wall);
    Cells = array [0 .. MAXPOS] of CellType;
    TargetCells = array [0 .. MAXPOS] of boolean;
    BoardType = record
      position : integer;
      cell : Cells;
      target : TargetCells;
    end;
    UndoPtr   = ^UndoEntry;
    UndoEntry = record
      action: ActionType;
      pushed: boolean;
      next: UndoPtr;
    end;

var
    action : ActionType;
    end_of_game : boolean;
    levels : array [1..MAXLEVEL] of BoardType;
    level : BoardType;    { active level }
    num_levels, current_level : integer;
    undo : UndoPtr;

procedure ExtRead(var ch: char; var ec: ExtendedChar);
begin
    ch := ReadKey;
    ec := no_key;
    if ch = #0 then
    begin
      ch := ReadKey;
      case ch of
        #72:
          ec := up_key;
        #75:
          ec := left_key;
        #77:
          ec := right_key;
        #80:
          ec := down_key;
      end;
      ch := #0;
   end;
end;

{ Display one cell }

procedure DisplayCell (var board: BoardType; pos: integer);
begin
  GotoXY(2*(pos mod OFFSET)+10, pos div OFFSET+5);
  with board do
    if pos = position then
      Write('@@')
    else
      case cell[pos] of
        empty:
          if target[pos] then
            Write('..')
          else
            Write('  ');
      wall:
        Write('##');
      box:
        if target[pos] then
          Write('{}')
        else
          Write('[]')
      end; {case}
end;

{ Display the complete board }

procedure DisplayBoard (var board: BoardType);
var
  i : integer;
BEGIN
  GotoXY(10,3);
  write('                             '); { delete line }
  for i := 0 to MAXPOS do
    begin
      DisplayCell(board,i);
    end;
  GotoXY(10,3);
  write('Nr. ');
  write(current_level:2);
  GotoXY(2,23);
  write('Cursor keys to move, Del to undo, ESC for menu');
  GotoXY(1,1);
end;

{ Perform 'action' on 'board' }

procedure Move (var board: BoardType; action: ActionType);
var
  diff	     : integer;
  local_undo : UndoPtr;

  procedure AddUndo (action: ActionType; pushed: boolean);
  { XXX better use TList: http://www.freepascal.org/docs-html/current/rtl/classes/tlist.html }
  var
    undo_entry : UndoPtr;
  begin
    NEW(undo_entry);
    undo_entry^.action := action;
    undo_entry^.pushed := pushed;
    undo_entry^.next := undo;
    undo := undo_entry;
  end;

begin
  with board do
    if action = undo_move then
      begin
        if undo <> NIL then
          begin
            case undo^.action of
              move_left:
                diff := -1;
              move_right:
                diff := 1;
              move_up:
                diff := -OFFSET;
              move_down:
                diff := OFFSET;
            end; {case}

            if undo^.pushed then
              begin
                cell [position+diff] := empty;
                cell [position] := box;
                DisplayCell(board,position+diff);
              end;

            position := position - diff;
            DisplayCell(board, position+diff);
            DisplayCell(board, position);
            local_undo := undo;
            undo := undo^.next;
            DISPOSE(local_undo);
          end; {if undo <> NIL}
      end
    else {if action <> undo_move}
      begin
        case action of
          move_left:
            diff := -1;
          move_right:
            diff := 1;
          move_up:
            diff := -OFFSET;
          move_down:
            diff := OFFSET;
        end; {case}

        if cell [position+diff] = empty then
          begin
            position := position+diff;
            DisplayCell(board, position);
            DisplayCell(board, position-diff);
            AddUndo (action,FALSE);
          end
        else
          if cell [position+diff] = box then
            if cell [position+2*diff] = empty then
              begin
                cell [position+2*diff] := box;
                cell [position+diff] := empty;
                position := position+diff;
                DisplayCell(board, position+diff);
                DisplayCell(board, position);
                DisplayCell(board, position-diff);
                AddUndo(action,TRUE);
              end;
    end; {if action <> undo_move}
  GotoXY(1,1);
end;

{ Reads a key and returns an action }

function Input () : ActionType;
var
  ch : char;
  ec : ExtendedChar;
  res : ActionType;
  ok : boolean;
begin
  ok := false;
  repeat
    ExtRead(ch,ec);
    if ch = esc then
      begin
        res := open_menu;
        ok := TRUE;
      end
    else if ch = del then
      begin
        res := undo_move;
        ok := TRUE;
      end
    else
      case ec of
        left_key:
          begin
            res := move_left;
            ok := TRUE;
          end;
        right_key:
          begin
            res := move_right;
            ok := TRUE;
          end;
        up_key:
          begin
            res := move_up;
            ok := TRUE;
          end;
        down_key:
          begin
            res := move_down;
            ok := TRUE;
          end;
      end; {case}
  until ok;
  input := res;
end;

procedure GotoLevel (nr: integer);
var
  local_undo : UndoPtr;
begin
  if nr < 1 then
    nr := 1
  else if nr > num_levels then
    nr := num_levels;

  current_level := nr;
  while undo <> NIL do
    begin
      local_undo := undo;
      undo := undo^.next;
      DISPOSE (local_undo);
    end;
  level := levels[current_level];
  DisplayBoard (level);
end;

function GetExecutableDir : String;
var
  program_name, path, dir : String;
  n : integer;
begin
  GetExecutableDir := '.';  { fallback result }

  try
    program_name := ParamStr(0);
    n := RPos(DirectorySeparator,program_name);
    { if 'program_name' contains a '/', return the string up to the last '/' }

    if n > 0 then
      begin
        GetExecutableDir := LeftStr(program_name,n-1);
      end
    else

    { otherwise split $PATH and test if 'program_name' is in any directory }

      begin
        path := GetEnvironmentVariable('PATH');
        if path <> '' then
	  begin
            n := 1;
 	    dir := ExtractWord(n,path,[PathSeparator]);
            while dir <> '' do
	      begin
	        dir := ExtractWord(n,path,[PathSeparator]);
                if FileExists(dir + '/' + program_name) then
	          begin
	            GetExecutableDir := dir;
		    break;
	          end;
                n := n+1;
	      end; {while}
	  end; {if path}
      end; {if n}

  except
    on e:Exception do
      begin
      end;
  end; {try}
end;

procedure LoadLevel(filename: String);
var
  i,j,a : integer;
  by1, by2 : byte;
  input_file : TFileStream;
begin
  try
    filename := GetExecutableDir + DirectorySeparator + filename;
    input_file := TFileStream.Create(filename,fmOpenRead);

    num_levels := 50; { there a 50 levels in sokoban.dat, see 'decode_map.rb' }

    for i := 1 to num_levels do
      begin
        with levels[i] do
          begin
            by1 := input_file.ReadByte;
            by2 := input_file.ReadByte;
            position := ORD(by1) + 256 * ORD(by2);

            for j := 0 to MAXPOS do
              begin
                by1 := input_file.ReadByte;
                a := ORD(by1);
                if (a=3) or (a=$17) then
                  target[j] := true
                else
                  target[j] := FALSE;

                if (a=0) or (a=3) then
                  cell[j] := empty
                else if a=1 then
                  cell[j] := wall
                else if a>=$14 then
                  cell[j] := box;
              end;
          end; {with}
      end; {while}

  except
    on e:Exception do
      begin
        writeln();
        writeln(filename,': ', e.message);
        GotoXY(1,24);
        halt;
      end;
  end; {try}

end;

procedure Init;
begin
   ClrScr;
   GotoXY(29,2);
   Write('---  S O K O B A N  ---');
   end_of_game := FALSE;
   undo := NIL;
   LoadLevel('sokoban.dat');
   GotoLevel(1);
end;

procedure DisplayMenu;
const
   x = 50;
   y = 5;
var
   wahl : char;
   i : integer;
   wert : cardinal;
   ec : ExtendedChar;
begin
   GotoXY(x,y);    Write('M e n u :');
   GotoXY(x,y+3);  Write('N .... Next Level');
   GotoXY(x,y+5);  Write('P .... Previous Level');
   GotoXY(x,y+7);  Write('G .... Goto Level');
   GotoXY(x,y+9);  Write('R .... Reset Level');
   GotoXY(x,y+11); Write('Q .... Quit Sokoban');
   GotoXY(x,y+13); Write('Choice:_');

   ExtRead(wahl,ec);
   wahl := UpCase(wahl);
   Write(wahl);
   case wahl of
     'N' : INC(current_level);
     'P' : DEC(current_level);
     'G' :
       begin
        GotoXY(x,y+16); Write('Enter Level: ');
        Read(wert);
        current_level := wert;
       end;
     'R' : begin end; { nothing, simply execute GotoLevel() }
     'Q' : end_of_game := true;
   end; {case}

   for i := y to y+16 do
      begin
        GotoXY(x,i);
        Write('                             ');
      end;

   if (wahl='N') or (wahl='P') or (wahl='G') or (wahl='R') then
      GotoLevel(current_level);
end;

{ main program }

begin
  Init;
  while not end_of_game do
    begin
      action := Input();
      if action in [move_left, move_right, move_up, move_down, undo_move] then
        Move(level, action)
      else
        DisplayMenu;
    end; {while}
    GotoXY(1,25);
end.
