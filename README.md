## My Backup

![icon](icons/86x86/harbour-mybackup.png)

Allows to add arbitrary files and dconf keys to Sailfish OS backups.
Requires Sailfish OS 3.2 or greater.

Sailfish OS applications can declare what they need to be added to
the backup by providing `[X-HarbourBackup]` section in their desktop
file, which looks like this:

```
[X-HarbourBackup]
BackupPathList=.local/share/foil/:Documents/FoilAuth/
BackupConfigList=/apps/harbour-foilauth/
```

`BackupPathList` is a colon-separated list of files and directories
to backup. Paths are relative to the home directory. Directory names
end with a slash and are copied recursively. Absolute paths are ignored.

`BackupConfigList` is a colon-separated list of dconf keys and groups.
Similarly to directories, group names end with a slash and are saved
and restored recursively. There is one important difference, though -
the existing contents of dconf groups is lost during restore and is
completely replaced by whatever was saved in the backup.

Stay safe, backup often!
