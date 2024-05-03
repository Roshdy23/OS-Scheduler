#include <gtk/gtk.h>

static void simulateClickHandler(GtkWidget *widget, gpointer data)
{
    // Retrieve text from the file path entry
    GtkWidget *filePathEntry = GTK_WIDGET(g_ptr_array_index((GPtrArray *)data, 0));
    const gchar *filePathText = gtk_entry_get_text(GTK_ENTRY(filePathEntry));

    // Retrieve text from the quantum entry
    GtkWidget *quantumEntry = GTK_WIDGET(g_ptr_array_index((GPtrArray *)data, 2));
    const gchar *quantum = gtk_entry_get_text(GTK_ENTRY(quantumEntry));

    // Retrieve selected option from the combo box
    GtkComboBox *AlgoComboBox = GTK_COMBO_BOX(g_ptr_array_index((GPtrArray *)data, 1));
    gint active_index = gtk_combo_box_get_active(GTK_COMBO_BOX(AlgoComboBox));

    if (active_index < 0 || g_utf8_strlen(filePathText, -1) == 0)
    {
        // Display an error message
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "Please select an Algorithm and enter a file path.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    gint q = 0;
    if (active_index == 1 && g_utf8_strlen(quantum, -1) == 0)
    {
        // Display an error message
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "Please enter quantum value.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    else
    {
        // Attempt to convert the entered text to an integer
        gchar *endptr;
        long int_value = strtol(quantum, &endptr, 10); // Base 10

        // Check if conversion was successful and if the entire string was consumed
        if (active_index == 1 && (*endptr != '\0' || endptr == quantum))
        {
            // Display an error message
            GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_ERROR,
                                                       GTK_BUTTONS_CLOSE,
                                                       "Please enter a valid integer for the quantum.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }
    g_print("simulating...\n");
    // Display the selected option
    g_print("File path entered: %s, Selected option: %d,quantum: %d\n", filePathText, active_index+1, atoi(quantum));
    char algstr[10];
    sprintf(algstr, "%d", (active_index + 1));
    execl("./process_generator.out", "process_generator.out", filePathText, algstr, quantum, NULL);
    return;
}

int main(int argc, char **argv)
{
    GtkApplication *app;
    int ret;

    gtk_init(&argc, &argv);
    GtkWidget *window;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_window_set_title(GTK_WINDOW(window), "Scheduler Simulator");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a vertical box to organize widgets
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create a horizontal box to center the grid horizontally
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 120);

    // Create a grid to organize widgets
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10); // Set column spacing to 10 pixels
    gtk_grid_set_row_spacing(GTK_GRID(grid), 20);    // Set row spacing to 10 pixels

    // Create a label for the input field
    GtkWidget *label2 = gtk_label_new("Algorithm:");
    gtk_grid_attach(GTK_GRID(grid), label2, 0, 0, 1, 1);

    // create a combo box for algorithms
    GtkWidget *AlgoComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(AlgoComboBox), "HPF");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(AlgoComboBox), "RR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(AlgoComboBox), "SRTN");
    gtk_grid_attach(GTK_GRID(grid), AlgoComboBox, 1, 0, 2, 1);

    // Create a label for the file path input field
    GtkWidget *label1 = gtk_label_new("Processes file path:");
    gtk_grid_attach(GTK_GRID(grid), label1, 0, 1, 1, 1);

    // Create file path input field
    GtkWidget *ProcessesFileEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), ProcessesFileEntry, 1, 1, 2, 1);

    // Create a label for the quantum input field
    GtkWidget *label3 = gtk_label_new("Quantum:");
    gtk_grid_attach(GTK_GRID(grid), label3, 0, 2, 1, 1);

    // Create quantum input field
    GtkWidget *quantumEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), quantumEntry, 1, 2, 2, 1);

    // Create simulate button
    GtkWidget *simulateButton = gtk_button_new_with_label("simulate");
    GPtrArray *data = g_ptr_array_new();
    g_ptr_array_add(data, ProcessesFileEntry);
    g_ptr_array_add(data, AlgoComboBox);
    g_ptr_array_add(data, quantumEntry);
    g_signal_connect(simulateButton, "clicked", G_CALLBACK(simulateClickHandler), data);
    gtk_grid_attach(GTK_GRID(grid), simulateButton, 1, 3, 1, 1);

    // Add spacers to hbox to center the grid horizontally
    GtkWidget *spacer_start = gtk_label_new(NULL);
    GtkWidget *spacer_end = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), spacer_start, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), grid, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), spacer_end, TRUE, TRUE, 0);

    // Set alignment of grid within hbox to center horizontally
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);

    gtk_widget_show_all(window);
    gtk_main();
    return ret;
}