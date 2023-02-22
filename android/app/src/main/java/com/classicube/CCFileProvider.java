package com.classicube;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.ProviderInfo;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;
import android.provider.OpenableColumns;

public class CCFileProvider extends ContentProvider
{
    final static String[] DEFAULT_COLUMNS = { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE, MediaStore.MediaColumns.DATA };
    File root;

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public void attachInfo(Context context, ProviderInfo info) {
        super.attachInfo(context, info);
        root = context.getExternalFilesDir(null); // getGameDataDirectory
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        File file = getFileForUri(uri);
        // can be null when caller is requesting all columns
        if (projection == null) projection = DEFAULT_COLUMNS;

        ArrayList<String> cols = new ArrayList<String>(3);
        ArrayList<Object> vals = new ArrayList<Object>(3);

        for (String column : projection) {
            if (column.equals(OpenableColumns.DISPLAY_NAME)) {
                cols.add(OpenableColumns.DISPLAY_NAME);
                vals.add(file.getName());
            } else if (column.equals(OpenableColumns.SIZE)) {
                cols.add(OpenableColumns.SIZE);
                vals.add(file.length());
            } else if (column.equals(MediaStore.MediaColumns.DATA)) {
                cols.add(MediaStore.MediaColumns.DATA);
                vals.add(file.getAbsolutePath());
            }
        }

        // https://stackoverflow.com/questions/4042434/converting-arrayliststring-to-string-in-java
        MatrixCursor cursor = new MatrixCursor(cols.toArray(new String[0]), 1);
        cursor.addRow(vals.toArray());
        return cursor;
    }

    @Override
    public String getType(Uri uri) {
        String path = uri.getEncodedPath();
        int sepExt  = path.lastIndexOf('.');

        if (sepExt >= 0) {
            String fileExt = path.substring(sepExt);
            if (fileExt.equals(".png")) return "image/png";
        }
        return "application/octet-stream";
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException("Readonly access");
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException("Readonly access");
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException("Readonly access");
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        File file = getFileForUri(uri);
        return ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
    }

    public static Uri getUriForFile(String path) {
        // See AndroidManifest.xml for authority
        return new Uri.Builder()
                .scheme("content")
                .authority("com.classicube.android.client.provider")
                .encodedPath(Uri.encode(path, "/"))
                .build();
    }

    File getFileForUri(Uri uri) {
        String path = uri.getPath();
        File file   = new File(root, path);

        file = file.getAbsoluteFile();
        // security validation check
        if (!file.getPath().startsWith(root.getPath())) {
            throw new SecurityException("Resolved path lies outside app directory:" + path);
        }
        return file;
    }
}
