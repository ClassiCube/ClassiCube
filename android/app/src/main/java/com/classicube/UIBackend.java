package com.classicube;
import java.util.ArrayList;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.text.Editable;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.style.ForegroundColorSpan;
import android.view.Gravity;
import android.view.ViewGroup;
import android.view.View;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

public class UIBackend
{
    public MainActivity activity;
    public UIBackend(MainActivity activity) {
        this.activity = activity;
    }

    CharSequence makeColoredText(String input) {
        int[] state = new int[2];
        SpannableStringBuilder sb = new SpannableStringBuilder();

        for (;;) {
            String left = MainActivity.nextTextPart(input, state);
            if (left.length() == 0) break;

            sb.append(left);
            sb.setSpan(new ForegroundColorSpan(state[1]),
                    sb.length() - left.length(), sb.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        return sb;
    }


    // ====================================================================
    // ----------------------------- 2D view ------------------------------
    // ====================================================================
    static int widgetID = 200;
    static final int _MATCH_PARENT = ViewGroup.LayoutParams.MATCH_PARENT;
    static final int _WRAP_CONTENT = ViewGroup.LayoutParams.WRAP_CONTENT;
    interface UICallback { void execute(); }

    public void create2DView_async() {
        activity.runOnUiThread(new Runnable() {
            public void run() { create2DView(); }
        });
    }

    void create2DView() {
        activity.launcher = true;
        activity.setActiveView(new CC2DLayout(activity));
        activity.pushCmd(MainActivity.CMD_UI_CREATED);
    }

    int showWidgetAsync(final View widget, final ViewGroup.LayoutParams lp,
                        final UICallback callback) {
        widget.setId(widgetID++);

        activity.runOnUiThread(new Runnable() {
            public void run() {
                CC2DLayout view = (CC2DLayout)activity.curView;
                if (callback != null) callback.execute();
                view.addView(widget, lp);
            }
        });
        return widget.getId();
    }

    void clearWidgetsAsync() {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                CC2DLayout view = (CC2DLayout)activity.curView;
                view.removeAllViews();
            }
        });
    }

    ViewGroup.LayoutParams makeLayoutParams(int xMode, int xOffset, int yMode, int yOffset,
                                            int width, int height) {
        return new CC2DLayoutParams(xMode, xOffset, yMode, yOffset, width, height);
    }

    // TODO reuse native code


    class CC2DLayout extends ViewGroup
    {
        public CC2DLayout(Context context) { super(context, null); }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int maxWidth  = 0;
            int maxHeight = 0;
            measureChildren(widthMeasureSpec, heightMeasureSpec);

            // Calculate bounds that encloses all children
            for (int i = 0; i < getChildCount(); i++)
            {
                View child = getChildAt(i);
                int childR = child.getLeft() + child.getMeasuredWidth();
                int childB = child.getTop()  + child.getMeasuredHeight();

                maxWidth   = Math.max(maxWidth,  childR);
                maxHeight  = Math.max(maxHeight, childB);
            }

            setMeasuredDimension(
                    resolveSizeAndState(maxWidth,  widthMeasureSpec,  0),
                    resolveSizeAndState(maxHeight, heightMeasureSpec, 0));
        }

        @Override
        protected void onLayout(boolean changed, int l, int t, int r, int b) {
            for (int i = 0; i < getChildCount(); i++)
            {
                View child = getChildAt(i);
                int width  = child.getMeasuredWidth();
                int height = child.getMeasuredHeight();

                CC2DLayoutParams lp = (CC2DLayoutParams)child.getLayoutParams();
                int x = MainActivity.calcOffset(lp.xMode, lp.xOffset, width,  getWidth());
                int y = MainActivity.calcOffset(lp.yMode, lp.yOffset, height, getHeight());

                child.layout(x, y,x  + width, y + height);
            }

            // TODO should this be elsewhere?? in setFrame ??
            if (changed) redrawBackground();
        }

        @Override
        public boolean shouldDelayChildPressedState() { return false; }
    }

    static class CC2DLayoutParams extends ViewGroup.LayoutParams
    {
        public int xMode, xOffset;
        public int yMode, yOffset;

        public CC2DLayoutParams(int xm, int xo, int ym, int yo, int width, int height) {
            super(width, height);
            xMode = xm; xOffset = xo;
            yMode = ym; yOffset = yo;
        }
    }


    // ====================================================================
    // --------------------------- 2D widgets -----------------------------
    // ====================================================================
    int buttonAdd(int xMode, int xOffset, int yMode, int yOffset,
                  int width, int height) {
        final Button btn = new Button(activity);
        final ViewGroup.LayoutParams lp = makeLayoutParams(xMode, xOffset, yMode, yOffset,
                width, height);

        buttonUpdateBackground(btn, width, height);
        btn.setTextColor(Color.WHITE);
        btn.setPadding(btn.getPaddingLeft(), 0, btn.getPaddingRight(), 0);
        btn.setTransformationMethod(null); // get rid of all caps

        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.pushCmd(MainActivity.CMD_UI_CLICKED, v.getId());
            }
        });
        return showWidgetAsync(btn, lp, null);
    }

    static void buttonUpdateBackground(Button btn, int width, int height) {
        // https://stackoverflow.com/questions/5092649/android-how-to-update-the-selectorstatelistdrawable-programmatically
        StateListDrawable sld = new StateListDrawable();
        sld.addState(new int[] { android.R.attr.state_pressed }, buttonMakeImage(btn, width, height, true));
        sld.addState(new int[] {}, buttonMakeImage(btn, width, height, false));
        btn.setBackgroundDrawable(sld);
    }


    static Drawable buttonMakeImage(Button btn, int width, int height, boolean hovered) {
        Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

        if (hovered) {
            MainActivity.makeButtonActive(bmp);
        } else {
            MainActivity.makeButtonDefault(bmp);
        }

        return new BitmapDrawable(bmp);
    }

    void buttonUpdate(final int id, final String text) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                View view = activity.findViewById(id);
                if (view == null) return;

                ((Button)view).setText(makeColoredText(text));
            }
        });
    }

    void buttonUpdateBackground(final int id) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                View view = activity.findViewById(id);
                if (view == null) return;

                buttonUpdateBackground((Button)view, view.getWidth(), view.getHeight());
            }
        });
    }

    int labelAdd(int xMode, int xOffset, int yMode, int yOffset) {
        final TextView lbl = new TextView(activity);
        final ViewGroup.LayoutParams lp = makeLayoutParams(xMode, xOffset, yMode, yOffset,
                _WRAP_CONTENT, _WRAP_CONTENT);
        lbl.setTextColor(Color.WHITE);

        return showWidgetAsync(lbl, lp, null);
    }

    void labelUpdate(final int id, final String text) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                View view = activity.findViewById(id);
                if (view == null) return;

                ((TextView)view).setText(makeColoredText(text));
            }
        });
    }

    int inputAdd(int xMode, int xOffset, int yMode, int yOffset,
                 int width, int height, int flags, String placeholder) {
        final EditText ipt = new EditText(activity);
        final ViewGroup.LayoutParams lp = makeLayoutParams(xMode, xOffset, yMode, yOffset,
                width, height);
        ipt.setBackgroundColor(Color.WHITE);
        ipt.setPadding(ipt.getPaddingLeft(), 0, ipt.getPaddingRight(), 0);
        ipt.setHint(placeholder);
        ipt.setInputType(MainActivity.calcKeyboardType(flags));
        ipt.setImeOptions(MainActivity.calcKeyboardOptions(flags));

        ipt.addTextChangedListener(new TextWatcher() {
            public void afterTextChanged(Editable s) { }
            public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

            public void onTextChanged(CharSequence s, int start, int before, int count) {
                activity.pushCmd(MainActivity.CMD_UI_STRING, ipt.getId(), s.toString());
            }
        });
        return showWidgetAsync(ipt, lp, null);
    }

    void inputUpdate(final int id, final String text) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                View view = activity.findViewById(id);
                if (view != null) { ((EditText)view).setText(text); }
            }
        });
    }

    int lineAdd(int xMode, int xOffset, int yMode, int yOffset,
                int width, int height, int color) {
        final View view = new View(activity);
        final ViewGroup.LayoutParams lp = makeLayoutParams(xMode, xOffset, yMode, yOffset,
                width, height);
        view.setBackgroundColor(color);

        return showWidgetAsync(view, lp, null);
    }

    int checkboxAdd(int xMode, int xOffset, int yMode, int yOffset,
                    String title, final boolean checked) {
        final CheckBox cb = new CheckBox(activity);
        final CC2DLayoutParams lp = new CC2DLayoutParams(xMode, xOffset, yMode, yOffset,
                _WRAP_CONTENT, _WRAP_CONTENT);
        cb.setText(title);
        cb.setTextColor(Color.WHITE);

        return showWidgetAsync(cb, lp, new UICallback() {
            public void execute() {
                // setChecked can't be called on render thread
                //  (setChecked triggers an animation)
                cb.setChecked(checked);

                cb.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton v, boolean isChecked) {
                        activity.pushCmd(MainActivity.CMD_UI_CHANGED, v.getId(), isChecked ? 1 : 0);
                    }
                });
            }
        });
    }

    void checkboxUpdate(final int id, final boolean checked) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                View view = activity.findViewById(id);
                if (view != null) { ((CheckBox)view).setChecked(checked); }
            }
        });
    }

    int sliderAdd(int xMode, int xOffset, int yMode, int yOffset,
                  int width, int height, int color) {
        final ProgressBar prg = new ProgressBar(activity, null, android.R.attr.progressBarStyleHorizontal);
        final ViewGroup.LayoutParams lp = makeLayoutParams(xMode, xOffset, yMode, yOffset,
                width, height);
        // https://stackoverflow.com/questions/39771796/change-horizontal-progress-bar-color
        prg.getProgressDrawable().setColorFilter(color, android.graphics.PorterDuff.Mode.SRC_IN);

        return showWidgetAsync(prg, lp, null);
    }

    void sliderUpdate(final int id, final int progress) {
        activity.runOnUiThread(new Runnable() {
            public void run() {
                View view = activity.findViewById(id);
                if (view != null) { ((ProgressBar)view).setProgress(progress); }
            }
        });
    }

    int tableAdd(int xMode, int xOffset, int yMode, int yOffset,
                 int color, int offset) {
        final ListView list = new ListView(activity);
        final ViewGroup.LayoutParams lp = makeLayoutParams(xMode, xOffset, yMode, yOffset,
                _MATCH_PARENT, _WRAP_CONTENT); //_MATCH_PARENT
        list.setPadding(0, 0, 0, offset);
        //list.setBackgroundColor(color);
		/*list.setScrollIndicators(
				View.SCROLL_INDICATOR_TOP |
				View.SCROLL_INDICATOR_BOTTOM |
				View.SCROLL_INDICATOR_LEFT |
				View.SCROLL_INDICATOR_RIGHT |
				View.SCROLL_INDICATOR_START |
				View.SCROLL_INDICATOR_END);
		list.setScrollbarFadingEnabled(false);*/
        final CCTableAdapter adapter = new CCTableAdapter(activity);
        // https://stackoverflow.com/questions/2454337/why-is-my-onitemselectedlistener-not-called-in-a-listview

        list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            View prev;

            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (prev != null) {
                    int prevPos = list.getPositionForView(prev);
                    if (prevPos >= 0)
                        prev.setBackgroundColor(adapter.getRowColor(prevPos, false));
                }

                prev = view;
                view.setBackgroundColor(adapter.getRowColor(position, true));
            }
        });

        return showWidgetAsync(list, lp, new UICallback() {
            public void execute() {
                list.setAdapter(adapter);
            }
        });
    }


    class TableEntry { public String title, details; public boolean featured; public Bitmap flag; }
    static ArrayList<TableEntry> table_entries = new ArrayList<TableEntry>();
    void tableStartUpdate() {
        table_entries.clear();
    }

    void tableAddEntry(String name, String details, boolean featured, Bitmap flag) {
        TableEntry e = new TableEntry();
        e.title    = name;
        e.details  = details;
        e.featured = featured;
        e.flag     = flag;
        table_entries.add(e);
    }

    void tableFinishUpdate(final int id) {
        final Object[] entries = table_entries.toArray();
        activity.runOnUiThread(new Runnable() {
            public void run() {
                View view = activity.findViewById(id);
                if (view == null) return;

                ListAdapter adapter = ((ListView)view).getAdapter();
                ((CCTableAdapter)adapter).entries = entries;
                ((CCTableAdapter)adapter).notifyDataSetChanged();
            }
        });
    }


    class CCTableAdapter extends BaseAdapter
    {
        public Object[] entries = new Object[0];
        Context ctx;

        public CCTableAdapter(Context context) {
            ctx	= context;
        }

        @Override
        public int getCount() { return entries.length; }

        @Override
        public String getItem(int position) { return ""; }

        @Override
        public long getItemId(int position) { return position; }

        // Offsets were calculated by trying to match iOS appearance
        int Pixels(int x) { return (int)(activity.getDpiX() * x); }
        static final int FLAG_WIDTH  = 16;
        static final int FLAG_HEIGHT = 11;

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null)
                convertView = createRow(position);

            LinearLayout root = (LinearLayout)convertView;
            LinearLayout text = (LinearLayout)root.getChildAt(1);
            TextView title	= (TextView)text.getChildAt(0);
            TextView details  = (TextView)text.getChildAt(1);

            TableEntry entry  = (TableEntry)entries[position];
            title.setText(entry.title);
            details.setText(entry.details);

            ImageView flag = (ImageView)root.getChildAt(0);
            if (entry.flag != null) flag.setImageBitmap(entry.flag);

            int width = flag.getMeasuredWidth();
            int height = flag.getMeasuredHeight();
            convertView.setBackgroundColor(getRowColor(position, false));
            return convertView;
        }

        public int getRowColor(int position, boolean selected) {
            TableEntry entry = (TableEntry)entries[position];
            return MainActivity.tableGetColor(position, selected, entry.featured);
        }

        View createRow(int position) {
            ImageView image = new ImageView(ctx);
            LinearLayout.LayoutParams imageLP = new LinearLayout.LayoutParams(
                    Pixels(FLAG_WIDTH), Pixels(FLAG_HEIGHT));
            imageLP.gravity = Gravity.CENTER;

            TextView title = new TextView(ctx);
            title.setSingleLine(true);
            title.setEllipsize(TextUtils.TruncateAt.END);
            title.setPadding(Pixels(8), Pixels(8), 0, 0);
            title.setTextSize(16);
            title.setTextColor(Color.WHITE);

            TextView details = new TextView(ctx);
            details.setSingleLine(true);
            details.setEllipsize(TextUtils.TruncateAt.END);
            details.setPadding(Pixels(8), Pixels(4), 0, Pixels(8));
            details.setTextSize(14);
            details.setTextColor(Color.WHITE);

            LinearLayout text = new LinearLayout(ctx);
            text.setOrientation(LinearLayout.VERTICAL);
            text.addView(title, new LinearLayout.LayoutParams(_MATCH_PARENT, _WRAP_CONTENT));
            text.addView(details, new LinearLayout.LayoutParams(_MATCH_PARENT, _WRAP_CONTENT));

            LinearLayout root = new LinearLayout(ctx);
            root.setPadding(Pixels(7), 0, Pixels(15), 0);
            root.addView(image, imageLP);
            root.addView(text, new LinearLayout.LayoutParams(_MATCH_PARENT, _WRAP_CONTENT));
            return root;
        }
    }


    // ======================================================================
    // ----------------------------- UI Backend -----------------------------
    // ======================================================================
    public void redrawBackground() {
        int width  = Math.max(1, activity.curView.getWidth());
        int height = Math.max(1, activity.curView.getHeight());
        Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        MainActivity.drawBackground(bmp);

        final BitmapDrawable drawable = new BitmapDrawable(bmp);
        activity.runOnUiThread(new Runnable() {
            public void run() { activity.curView.setBackgroundDrawable(drawable); }
        });
    }
}