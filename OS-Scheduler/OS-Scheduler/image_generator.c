#include <gtk/gtk.h>
//------------------count lines of file-------------------//
int countLines(char *filePath)
{
    FILE *f = fopen(filePath, "r");
    int ch = 0;
    int lines = 0;

    if (f == NULL)
        return 0;

    lines++;
    while ((ch = fgetc(f)) != EOF)
    {
        if (ch == '\n')
            lines++;
    }
    fclose(f);
    return lines;
}
//----------------generate image---------------//
static void convertTxttoImage(char *filePath, char *imageName)
{
    int linesNum = countLines(filePath);

    // Create a Cairo surface
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 15 * 65, 25 * (linesNum) + 80); // Adjust dimensions as needed

    // Create a Cairo context
    cairo_t *cr = cairo_create(surface);

    // Set background color to white
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White color
    cairo_paint(cr);

    // Set font size and position
    cairo_set_font_size(cr, 20);
    cairo_move_to(cr, 50, 50);

    // Draw text and selected option
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black color for text

    FILE *file = fopen(filePath, "r");
    char line[100];

    if (file == NULL)
    {
        printf("Error in opening the file .");
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        return;
    }

    while (fgets(line, sizeof(line), file))
    {
        double x;
        double y;

        char *newline_pos = strchr(line, '\n'); // Find the position of the newline character
        if (newline_pos != NULL)
        {
            *newline_pos = '\0'; // Replace newline character with null terminator
        }

        cairo_show_text(cr, line);
        cairo_get_current_point(cr, &x, &y); // Reset x-coordinate to 0
        cairo_move_to(cr, 50, y + 25);       // Move to the next line
    }

    // Save the surface as an image file
    cairo_surface_write_to_png(surface, imageName); // Specify the file name and path

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}
int main(int argc, char **argv)
{
    convertTxttoImage("scheduler.log", "scheduler.log.png");
    convertTxttoImage("scheduler.perf", "scheduler.perf.png");
    return 0;
}