PS3 HDD Reader final for Windows by picard(aka 3141card)

*******************************************************************************************
*                                                                                         *
*                                     Never forget!!!                                     *
*                                                                                         *
*                 If you connect a PS3 HDD to your Windows computer,                      *
*             windows will ask you whether the HDD should be initialized.                 *
*                                                                                         *
*                                    Refuse that!!!                                       *
*                                                                                         *
*        Or initialization will damage your PS3 HDD. Neither the PS3 HDD Reader           *
*                nor your PS3 will be able to read the HDD afterwards.                    *
*                                                                                         *
*******************************************************************************************

The program must be run as an administrator, otherwise there will be no raw access to HDDs.

The main purpose of this program is data recovery. Since the HDD of a PS3 is encrypted,
the program needs the "eid_root_key" of your PS3. This "eid_root_key" can only be dumped
on a PS3 with custom firmware. Use google to find out how to get your "eid_root_key".

Your "eid_root_key" must be in the program folder. All folders / files you dump will be
copied into the program folder. Make sure you have enough free space on your hard drive.

Open a command prompt as administrator and "cd" to the program folder. Run:

  ps3_hdd_reader.exe hdd

If everything is OK, a list of available partitions is displayed. The second argument "hdd"
indicates that we are using a PS3 HDD. If you have a dump of a PS3 HDD, place the file in
the program folder, rename the dump file to "backup.bin" and use "file" instead of "hdd".

As an example, the output for my PS3 Slim HDD is:
-------------------------------------------------------------------------------------------
available volumes are...

 dev_hdd0
 dev_hdd1
 dev_flash
 dev_flash2
 dev_flash3
-------------------------------------------------------------------------------------------

"dev_hdd0" is the gameOS partition. To see the contents of a partition, use the "dir" or
"ls" command. Run:

  ps3_hdd_reader.exe hdd dev_hdd0 dir /

to see the the contents of gameOS root. The output for my PS3 Slim HDD is:
-------------------------------------------------------------------------------------------
 Volume is ps3_hdd dev_hdd0
 Directory of dev_hdd0//

02.08.2021  05:52    <DIR>          .
02.08.2021  05:52    <DIR>          ..
01.21.2018  15:24    <DIR>          GAMES
01.21.2018  15:24    <DIR>          PKG
01.21.2018  14:59    <DIR>          crash_report
11.23.2016  01:10    <DIR>          data
11.27.2020  17:49    <DIR>          drm
04.26.2020  23:06    <DIR>          game
11.19.2012  09:58    <DIR>          game_debug
01.21.2018  14:34    <DIR>          home
01.21.2018  14:59    <DIR>          mms
01.21.2018  15:24    <DIR>          packages
03.05.2014  13:34    <DIR>          savedata
02.08.2021  05:50    <DIR>          tmp
05.17.2018  16:34    <DIR>          updater
04.26.2020  23:05    <DIR>          vm
08.31.2013  19:58    <DIR>          vsh
               0 File(s),  0 bytes
              17 Dir(s),  291.424.550.912 bytes free
-------------------------------------------------------------------------------------------

This is the content of my gameOS partition. Use the arrow keys up/down to get the last
command and expand the path to see the contents of a folder. Run:

  ps3_hdd_reader.exe hdd dev_hdd0 dir /home

The output for my home folder is:
-------------------------------------------------------------------------------------------
 Volume is ps3_hdd dev_hdd0
 Directory of dev_hdd0//home/

01.21.2018  14:34    <DIR>          .
02.08.2021  05:52    <DIR>          ..
05.10.2020  02:55    <DIR>          00000001
               0 File(s),  0 bytes
               3 Dir(s),  291.424.550.912 bytes free
-------------------------------------------------------------------------------------------

This is how you navigate through your fs.

If you want to copy something from the PS3 HDD to your PC, use the "copy" or "cp" command.
As an example I copy the PS2 emulator folder from the "dev_flash" partition. I run:

  ps3_hdd_reader.exe hdd dev_flash copy /ps2emu

and thats my output.
-------------------------------------------------------------------------------------------
copy -> /ps2emu/ps2_emu.self
copy -> /ps2emu/ps2_gxemu.self
copy -> /ps2emu/ps2_netemu.self
(100%)
-------------------------------------------------------------------------------------------

The folder "ps2emu" with the three ps2 emulators is now in my program folder. This way you
can dump all files from the available partitions.


Notice:
If the PS3 HDD is damaged, there is no guarantee that the PS3 HDD Reader will work or that
all files will be dumped without errors.

If my program could help you to save important data, I would be very happy about a donation :)

https://www.paypal.com/paypalme/3141card

picard (aka 3141card) 
