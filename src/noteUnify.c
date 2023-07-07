bool unifyNotes(UnifiedNote *unified, ParsedNote *parsed, Chart *chart) {
    unified->measure_whole = parsed->measure_whole;
    unified->measure_fraction = parsed->mesaure_fraction;
    unified->duration = parsed->duration;
    unified->location = parsed->location;
    unified->slide_id = parsed->slide_id;
    unified->slide_amount = parsed->slide_amount;

    switch (chart->type) {
        case ChartTypeSR:
            if ((char)parsed->type < '\0') {
                unified->note_type = NoteTypeStartSlide;
                unified->is_start_slide = true;
            } else if (parsed->slide_id == 0) {
                switch (parsed->type & 0xf) {
                    case 0:
                    case 1:
                        unified->note_type = NoteTypeTap;
                        break;
                    case 2:
                    case 3:
                        unified->note_type = NoteTypeHold;
                        break;
                    case 4:
                    case 5:
                        unified->note_type = NoteTypeBreak;
                }
            } else {
                unified->note_type = NoteTypeStar;
                unified->slide_amount = 1;
            }

            if (parsed->type & 0x40) {
                unified->field12_0x24 = 2.0;
            } else if (parsed->type & 0x20) {
                unified->field12_0x24 = 4.0;
            } else if (parsed->type & 0x10) {
                unified->field12_0x24 = 6.0;
            }

            if (parsed->slide_id == 0) {
                unified->slide_pattern = 0;
                return true;
            }

            unified->slide_amount = 0;
            unified->slide_delay = parsed->slide_delay;
            switch (parsed->slide_pattern) {
                case 0:
                    unified->slide_pattern = 1;
                    return true;
                case 1:
                    unified->slide_pattern = 3;
                    return true;
                case 2:
                    unified->slide_pattern = 2;
                    return true;
                default:
                    unified->slide_amount = 0;
                    return true;
            }

        case ChartTypeSZ:
            if (((char)parsed->type < '\0') || (parsed->type == NoteTypeStartSlide)) {
                unified->note_type = NoteTypeStartSlide;
                unified->is_start_slide = true;
            } else {
                unified->note_type = parsed->type;
            }
            unified->slide_pattern = parsed->slide_pattern;
            if (unified->note_type == NoteTypeStar) {
                unified->slide_amount = 1;
                unified->slide_delay = parsed->slide_delay;
                return true;
            }
            unified->slide_delay = parsed->slide_delay;
            return true;

        case ChartTypeSC:
        case ChartTypeSD:
            if ((-1 < (char)parsed->type) && (parsed->type != NoteTypeStartSlide)) {
                unified->note_type = parsed->type;
                unified->slide_pattern = parsed->slide_pattern;
                unified->slide_delay = parsed->slide_delay;
                return true;
            }
            unified->note_type = NoteTypeStartSlide;
            unified->is_start_slide = true;
            unified->slide_pattern = parsed->slide_pattern;
            unified->slide_delay = parsed->slide_delay;
            return true;

        default:
            return false;
    }
}