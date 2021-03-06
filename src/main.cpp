#include <iostream>

#include "base.hpp"
#include "game.hpp"
#include "assets.hpp"
#include "battle.hpp"

// Battle system process:
// 1. Every party member selects a ritual
// 2. Ritual minigames are executed
// 3.

GGJ16_NAMESPACE
{
    enum class battle_screen_state
    {
        player_menu,
        player_ritual,
        enemy_turn,
        before_enemy_turn,
        before_player_turn,
        to_next_ctx,
        to_game_over,
        game_over
    };

    enum class ritual_minigame_state
    {
        in_progress,
        failure,
        success,
        invalid
    };

    enum class ritual_type
    {
        resist,
        complete
    };

    class battle_ritual_context;

    namespace impl
    {
        class ritual_minigame_base
        {
        private:
            ritual_minigame_state _state{ritual_minigame_state::invalid};
            float _time_left;

        public:
            ritual_type _type{ritual_type::resist};

            battle_ritual_context* _ctx;
            game_app& app() noexcept;

            const auto& type() const noexcept { return _type; }

        protected:
            void failure() { _state = ritual_minigame_state::failure; }
            void success() { _state = ritual_minigame_state::success; }

        public:
            const auto& state() const noexcept { return _state; }

            virtual ~ritual_minigame_base() {}

            virtual void update(ft dt)
            {
                _time_left -= dt;
                if(_time_left <= 0.f)
                {
                    if(_type == ritual_type::resist)
                    {
                        success();
                    }
                    else if(_type == ritual_type::complete)
                    {
                        failure();
                    }
                }
            }

            virtual void draw() {}

            void start_minigame(float time_left)
            {
                _time_left = time_left;
                _state = ritual_minigame_state::in_progress;
            }

            const auto& time_left() const noexcept { return _time_left; }
        };
    }

    using ritual_uptr = std::unique_ptr<impl::ritual_minigame_base>;

    struct symbol_point
    {
        vec2f _p;
        float _radius{6.f};
    };


    class symbol_ritual : public impl::ritual_minigame_base
    {
    private:
        using base_type = impl::ritual_minigame_base;

        int next_id{1};
        vec2f _center{
            game_constants::width / 2.f, game_constants::height / 2.f};
        std::vector<sf::CircleShape> _pshapes;
        std::vector<ssvs::BitmapText> _ptexts;
        std::vector<int> _phits;

        auto is_shape_hovered(const sf::CircleShape& cs) noexcept
        {
            auto mp(app().mp());
            return ssvs::getDistEuclidean(mp, cs.getPosition()) <
                   cs.getRadius();
        }

        auto all_hit_before(int i)
        {
            for(int j = 0; j < i; ++j)
            {
                if(_phits[j] == 0) return false;
            }
            return true;
        }

        auto all_hit() const noexcept
        {
            for(const auto& h : _phits)
            {
                if(h == 0) return false;
            }
            return true;
        }

    public:
        void add_point(const symbol_point& sp)
        {
            // Add shape
            _pshapes.emplace_back();
            auto& s(_pshapes.back());
            s.setFillColor(sfc::Red);
            s.setOutlineThickness(2);
            s.setOutlineColor(sfc::Black);
            s.setRadius(sp._radius * 2.8f);
            ssvs::setOrigin(s, ssvs::getLocalCenter);
            s.setPosition(_center + sp._p);

            // Add text
            _ptexts.emplace_back(mkTxtOBSmall());
            auto& t(_ptexts.back());
            t.setString(std::to_string(next_id++));
            t.setScale(vec2f(3.f, 3.f));
            ssvs::setOrigin(t, ssvs::getLocalCenter);
            t.setPosition(s.getPosition());

            // Add hit
            _phits.emplace_back(0);
        }

        void update(ft dt) override
        {
            base_type::update(dt);

            if(all_hit())
            {
                success();
                return;
            }

            for(sz_t i = 0; i < _pshapes.size(); ++i)
            {
                auto& s(_pshapes[i]);
                auto& t(_ptexts[i]);
                auto& h(_phits[i]);

                if(all_hit_before(i) && is_shape_hovered(s))
                {
                    if(h == 0)
                    {
                        assets().psnd(assets().blip);
                        h = 1;
                    }
                }

                if(h == 1)
                {
                    s.setFillColor(sfc::Green);
                    t.setString("");
                    ssvs::setOrigin(s, ssvs::getLocalCenter);
                    s.setRadius(std::max(0.f, std::abs(s.getRadius() - (dt))));
                }
            }
        }
        void draw() override
        {
            for(auto& s : _pshapes)
            {
                app().render(s);
            }

            for(auto& t : _ptexts)
            {
                app().render(t);
            }
        }
    };

    class aura_ritual : public impl::ritual_minigame_base
    {
    private:
        using base_type = impl::ritual_minigame_base;

        vec2f _center{
            game_constants::width / 2.f, game_constants::height / 2.f};
        std::vector<sf::CircleShape> _pshapes;

        auto is_shape_hovered(const sf::CircleShape& cs) noexcept
        {
            auto mp(app().mp());
            return ssvs::getDistEuclidean(mp, cs.getPosition()) <
                   cs.getRadius();
        }

        auto any_dead() const noexcept
        {
            for(const auto& c : _pshapes)
                if(c.getRadius() < 5.f) return true;

            return false;
        }

    public:
        void add_point(const symbol_point& sp)
        {
            // Add shape
            _pshapes.emplace_back();
            auto& s(_pshapes.back());
            s.setFillColor(sfc::Red);
            s.setOutlineThickness(3);
            s.setOutlineColor(sfc::Black);
            s.setRadius(sp._radius * 3.1f);
            ssvs::setOrigin(s, ssvs::getLocalCenter);
            s.setPosition(_center + sp._p);
        }

        void update(ft dt) override
        {
            base_type::update(dt);

            if(any_dead())
            {
                failure();
            }

            bool mps{false};

            for(sz_t i = 0; i < _pshapes.size(); ++i)
            {
                auto& s(_pshapes[i]);

                ssvs::setOrigin(s, ssvs::getLocalCenter);


                if(is_shape_hovered(s))
                {
                    mps = true;
                    s.setFillColor(sfc::Green);
                    s.setRadius(
                        std::min(75.f, std::abs(s.getRadius() + dt * 2.78f)));
                }
                else
                {
                    s.setFillColor(sfc::Red);
                    s.setRadius(
                        std::max(0.f, std::abs(s.getRadius() - (dt * 0.74f))));
                }
            }

            if(mps)
            {
                assets().psnd(assets().bip);
            }
        }
        void draw() override
        {
            for(auto& s : _pshapes)
            {
                app().render(s);
            }
        }
    };



    class drag_ritual : public impl::ritual_minigame_base
    {
    private:
        using base_type = impl::ritual_minigame_base;

        vec2f _center{
            game_constants::width / 2.f, game_constants::height / 2.f};

        std::vector<sf::RectangleShape> _ptargets;
        std::vector<int> _phits;
        std::vector<sf::CircleShape> _pdraggables;
        int _curr = -1;


        auto is_shape_hovered(const sf::RectangleShape& cs) noexcept
        {
            auto mp(app().mp());

            if(mp.x < ssvs::getGlobalLeft(cs)) return false;
            if(mp.x > ssvs::getGlobalRight(cs)) return false;

            if(mp.y < ssvs::getGlobalTop(cs)) return false;
            if(mp.y > ssvs::getGlobalBottom(cs)) return false;

            return true;
        }

        auto is_shape_hovered(const sf::CircleShape& cs) noexcept
        {
            auto mp(app().mp());
            return ssvs::getDistEuclidean(mp, cs.getPosition()) <
                   cs.getRadius();
        }

        auto is_in_target(
            const sf::CircleShape& cs, const sf::RectangleShape rs)
        {
            return ssvs::getDistEuclidean(cs.getPosition(), rs.getPosition()) <
                   20.f * 3.f;
        }

        auto all_hit() const noexcept
        {
            for(const auto& x : _phits)
                if(x == 0) return false;
            return true;
        }


    public:
        void add_target(vec2f p)
        {
            _ptargets.emplace_back();
            auto& t(_ptargets.back());

            t.setFillColor(sfc::Black);
            t.setOutlineColor(sfc::White);
            t.setOutlineThickness(10.f);
            t.setSize(vec2f(26 * 3.f, 26 * 3.f));
            ssvs::setOrigin(t, ssvs::getLocalCenter);
            t.setPosition(p);
        }

        void add_draggable(vec2f p)
        {
            _pdraggables.emplace_back();
            _phits.emplace_back(0);
            auto& t(_pdraggables.back());

            t.setFillColor(sfc::Red);
            t.setOutlineColor(sfc::Black);
            t.setOutlineThickness(4.f);
            t.setRadius(7.f * 3.f);
            ssvs::setOrigin(t, ssvs::getLocalCenter);
            t.setPosition(p);
        }

        void update(ft dt) override
        {
            base_type::update(dt);

            if(all_hit())
            {
                success();
            }


            if(_curr != -1 && _phits[_curr] == 1)
            {
                _curr = -1;
            }

            for(int i = 0; i < (int)_pdraggables.size(); ++i)
            {
                auto& s(_pdraggables[i]);
                ssvs::setOrigin(s, ssvs::getLocalCenter);

                if(_phits[i] == 0 && is_shape_hovered(s) && _curr == -1)
                {
                    s.setFillColor(sfc::Green);
                    _curr = i;
                    assets().psnd(assets().bip);
                }

                if(_curr == i && _phits[i] == 0)
                {
                    auto mp(app().mp());
                    s.setPosition(vec2f(mp));
                }

                for(auto& t : _ptargets)
                {
                    if(is_in_target(s, t) && _phits[i] == 0)
                    {
                        s.setRadius(0.f);
                        _phits[i] = 1;
                        assets().psnd(assets().blip);

                        if(_curr == i)
                        {
                            _curr = -1;
                        }
                    }
                }
            }

            for(auto& t : _ptargets)
            {
                t.setRotation(t.getRotation() + dt * 0.5f);
            }
        }

        void draw() override
        {
            for(auto& s : _ptargets)
            {
                app().render(s);
            }

            for(auto& s : _pdraggables)
            {
                app().render(s);
            }
        }
    };

    class battle_participant;

    using ritual_effect_fn = std::function<void(battle_context_t&)>;

    class ritual_maker
    {
    private:
        std::string _label;
        std::string _desc;
        ritual_type _type;
        float _time;
        float _req_mana{0};
        std::function<ritual_uptr()> _fn_make;
        ritual_effect_fn _fn_effect;

    public:
        template <typename TF0, typename TF1>
        ritual_maker(const std::string& label, const std::string& desc,
            ritual_type type, float time, float req_mana, TF0&& f, TF1&& f_eff)
            : _label{label}, _desc{desc}, _type{type}, _time{time},
              _req_mana{req_mana}, _fn_make(f), _fn_effect(f_eff)
        {
        }

        const auto& type() const noexcept { return _type; }
        const auto& time() const noexcept { return _time; }
        const auto& label() const noexcept { return _label; }
        auto desc() const noexcept
        {
            std::ostringstream oss;
            oss << _desc << "\nTime: " << _time << "\tMana: " << _req_mana;
            return oss.str();
        }

        auto& req_mana() noexcept { return _req_mana; }
        const auto& req_mana() const noexcept { return _req_mana; }

        auto make() { return _fn_make(); }
        auto& effect() { return _fn_effect; }
    };

    class battle_screen;

    struct cenemy_state
    {
        sf::Texture* _enemy_texture;
        std::function<void(battle_screen&, battle_context_t&)> _f_ai;

        cenemy_state(sf::Texture* t) : _enemy_texture(t) {}
    };

    class cplayer_state
    {
    private:
        std::vector<ritual_maker> _atk_rituals;
        std::vector<ritual_maker> _utl_rituals;
        std::vector<ritual_maker> _mana_rituals;
        // vector of available items

    public:
        cplayer_state()
        {
            _atk_rituals.reserve(3);
            _utl_rituals.reserve(3);
            _mana_rituals.reserve(1);
        }

        template <typename T, typename TF0, typename TF1, typename... Ts>
        void emplace_atk_ritual(const std::string& label,
            const std::string& desc, ritual_type rt, float time, float req_mana,
            TF0&& f_init, TF1&& f_effect)
        {
            _atk_rituals.emplace_back(label, desc, rt, time, req_mana,
                [f_init, rt]
                {
                    auto uptr(std::make_unique<T>());
                    uptr->_type = rt;
                    f_init(*uptr);
                    return uptr;
                },
                f_effect);
        }

        template <typename T, typename TF0, typename TF1, typename... Ts>
        void emplace_utl_ritual(const std::string& label,
            const std::string& desc, ritual_type rt, float time, float req_mana,
            TF0&& f_init, TF1&& f_effect)
        {
            _utl_rituals.emplace_back(label, desc, rt, time, req_mana,
                [f_init, rt]
                {
                    auto uptr(std::make_unique<T>());
                    uptr->_type = rt;
                    f_init(*uptr);
                    return uptr;
                },
                f_effect);
        }

        template <typename T, typename TF0, typename TF1, typename... Ts>
        void emplace_mana_ritual(const std::string& label,
            const std::string& desc, ritual_type rt, float time, float req_mana,
            TF0&& f_init, TF1&& f_effect)
        {
            _mana_rituals.emplace_back(label, desc, rt, time, req_mana,
                [f_init, rt]
                {
                    auto uptr(std::make_unique<T>());
                    uptr->_type = rt;
                    f_init(*uptr);
                    return uptr;
                },
                f_effect);
        }

        template <typename TF>
        void for_atk_rituals(TF&& f)
        {
            for(auto& r : _atk_rituals) f(r);
        }

        template <typename TF>
        void for_utl_rituals(TF&& f)
        {
            for(auto& r : _utl_rituals) f(r);
        }

        template <typename TF>
        void for_mana_rituals(TF&& f)
        {
            for(auto& r : _mana_rituals) f(r);
        }
    };


    class battle_ritual_context
    {
    private:
        game_app& _app;
        std::unique_ptr<impl::ritual_minigame_base> _minigame{nullptr};
        ssvs::BitmapTextRich _tr{*assets().fontObStroked};
        ssvs::BTR::PtrChunk _ptr_time_text;

        auto valid() const noexcept
        {
            if(_minigame == nullptr) return false;
            if(_minigame->state() == ritual_minigame_state::invalid)
                return false;

            return true;
        }

        void init_text()
        {
            _tr.eff<BTR::Tracking>(-3)
                .eff(sfc::White)
                .in("Time left: ")
                .eff(sfc::Red)
                .in(_ptr_time_text)
                .mk("");

            _tr.setScale(vec2f(3.f, 3.f));
        }

    public:
        auto& app() noexcept { return _app; }

        battle_ritual_context(game_app& app) : _app(app) { init_text(); }

        auto is_failure() const noexcept
        {
            VRM_CORE_ASSERT(valid());
            return _minigame->state() == ritual_minigame_state::failure;
        }

        auto is_in_progress() const noexcept
        {
            VRM_CORE_ASSERT(valid());
            return _minigame->state() == ritual_minigame_state::in_progress;
        }

        auto is_success() const noexcept
        {
            VRM_CORE_ASSERT(valid());
            return _minigame->state() == ritual_minigame_state::success;
        }

        template <typename T>
        void set_and_start_minigame(float time_as_ft, T&& x)
        {
            _minigame = std::move(x);
            _minigame->start_minigame(time_as_ft);
            _minigame->_ctx = this;
        }

        void update(ft dt)
        {
            VRM_CORE_ASSERT(valid());

            _ptr_time_text->setStr(std::to_string(
                vrmc::to_int(ssvu::getFTToSeconds(_minigame->time_left()))));

            _tr.update(dt);
            _minigame->update(dt);
        }

        void draw()
        {
            VRM_CORE_ASSERT(valid());
            _minigame->draw();

            ssvs::setOrigin(_tr, ssvs::getLocalCenterE);
            _tr.setAlign(ssvs::TextAlign::Right);
            _tr.setPosition(vec2f(game_constants::width - 20, 20));
            _app.render(_tr);
        }
    };

    namespace impl
    {
        game_app& ritual_minigame_base::app() noexcept
        {
            assert(_ctx != nullptr);
            return _ctx->app();
        }
    }

    namespace impl
    {
        class single_stat_display
        {
        private:
            ssvs::BitmapText _t{mkTxtOBSmall()};

        public:
        };
    }

    class msgbox_screen : public game_screen
    {
    private:
        using base_type = game_screen;

        sf::RectangleShape _bg;
        ssvs::BitmapTextRich _btr{*assets().fontObStroked};
        float _safety_time{70};

        void init_bg()
        {
            _bg.setSize(vec2f(220 * 3, 120 * 3));
            _bg.setFillColor(sfc::Black);
            _bg.setOutlineColor(sfc::White);
            _bg.setOutlineThickness(3);
            ssvs::setOrigin(_bg, ssvs::getLocalCenter);
            _bg.setPosition(
                game_constants::width / 2.f, game_constants::height / 2.f);
        }

        void init_btr()
        {
            _btr.setAlign(ssvs::TextAlign::Center);
            _btr.setScale(vec2f(3.f, 3.f));
        }

    public:
        msgbox_screen(game_app& app) noexcept : base_type(app)
        {
            init_bg();
            init_btr();
        }

        auto& btr() noexcept { return _btr; }

        void update(ft dt) override
        {
            ssvs::setOrigin(_btr, ssvs::getLocalCenter);
            _btr.setPosition(vec2f(
                game_constants::width / 2.f, game_constants::height / 2.f));
            _btr.update(dt);

            if(_safety_time >= 0)
            {
                _safety_time -= dt;
            }
            else if(app().lb_down())
            {
                kill();
                app().pop_screen();
            }
        }

        void draw() override
        {
            app().render(_bg);
            app().render(_btr);
        }
    };

    class stat_bar : public sf::Transformable, public sf::Drawable
    {
    private:
        ssvs::BitmapText _txt{mkTxtOBSmall()};
        sf::RectangleShape _bar, _bar_outl;

        static constexpr float _bar_h{40.f};
        static constexpr float _bar_w_max{200.f};

    public:
        stat_bar(sf::Color bar_color)
        {
            _bar.setOutlineColor(sfc::Black);
            _bar.setOutlineThickness(3);
            _bar.setFillColor(bar_color);

            _bar_outl.setOutlineColor(sfc::Black);
            _bar_outl.setOutlineThickness(3);
            _bar_outl.setFillColor(sfc::Transparent);

            _txt.setScale(vec2f(3.f, 3.f));
        }

        void refresh(float value, float maxvalue)
        {
            auto width(value / maxvalue * _bar_w_max);
            _bar.setSize(vec2f(width, _bar_h));
            _bar_outl.setSize(vec2f(_bar_w_max, _bar_h));

            _txt.setString(std::to_string((int)value) + std::string{" / "} +
                           std::to_string((int)maxvalue));
            _txt.setPosition(_bar.getPosition());
        }

        virtual void draw(
            sf::RenderTarget& target, sf::RenderStates states) const override
        {
            states.transform *= getTransform();
            target.draw(_bar_outl, states);
            target.draw(_bar, states);
            target.draw(_txt, states);
        }
    };

    struct stats_gfx : public sf::Transformable, public sf::Drawable
    {
        stat_bar _health_b{sf::Color{255, 0, 0, 200}};
        stat_bar _shield_b{sf::Color{255, 255, 0, 200}};
        stat_bar _mana_b{sf::Color{0, 0, 255, 200}};
        bool _mana_visibile{true};

        stats_gfx()
        {
            _shield_b.setPosition(0, 120.f);
            _mana_b.setPosition(0, 60.f);
        }

        virtual void draw(
            sf::RenderTarget& target, sf::RenderStates states) const override
        {
            states.transform *= getTransform();
            target.draw(_health_b, states);
            target.draw(_shield_b, states);
            if(_mana_visibile) target.draw(_mana_b, states);
        }

        void refresh(const character_stats& cs)
        {
            _health_b.refresh(cs.health(), cs.maxhealth());
            _shield_b.refresh(cs.shield(), cs.maxshield());
            _mana_b.refresh(cs.mana(), cs.maxmana());
        }

        void hide_mana()
        {
            _mana_visibile = false;
            _shield_b.setPosition(0, 60.f);
        }
    };



    class battle_screen : public game_screen
    {
    public:
        struct scripted_event
        {
            float _time;
            std::function<void(float, ft)> _f_update;
            std::function<void()> _f_draw;

            void update(ft dt)
            {
                _f_update(_time, dt);
                _time -= dt;
            }
        };

        sf::Sprite _landscape{*assets().landscape};
        sf::Sprite _enemy;
        sf::Sprite _bar{*assets().bar};

        float _enemy_f{0};
        float _enemy_f_magnitude{30.f};

        float _enemy_shake{0};

        using base_type = game_screen;

        battle_menu _menu;
        battle_menu_gfx_state _menu_gfx_state;

        battle_menu_screen* _m_main;
        battle_menu_screen* _m_ritual_atk;
        battle_menu_screen* _m_ritual_utl;


        std::vector<std::unique_ptr<battle_context_t>> _ctxs;
        sz_t _ctx_idx{0};

        auto& curr_bctx() { return *(_ctxs[_ctx_idx]); }

        battle_screen_state _state{battle_screen_state::player_menu};

        battle_ritual_context _ritual_ctx;

        std::vector<scripted_event> _scripted_events;

        ssvs::BitmapTextRich _t_cs{*assets().fontObBig};
        ssvs::BTR::PtrChunk _ptr_t_cs;
        ssvs::BTR::PtrWave _ptr_t_cs_wave;

        ritual_effect_fn _success_effect;


        void reset()
        {
            _state = battle_screen_state::player_menu;
            _ctx_idx = 0;
        }

        stats_gfx _player_stats_gfx;
        stats_gfx _enemy_stats_gfx;

        vec2f _esprite_pos;

        void init_menu_bg()
        {
            auto sw(350.f);
            auto h(200.f);

            _bar.setPosition(vec2f{0, game_constants::height - h});

            _player_stats_gfx.setPosition(
                vec2f{85.f, game_constants::height - h + 20.f});
            _enemy_stats_gfx.setPosition(vec2f{20.f, 20.f});
            _enemy_stats_gfx.hide_mana();

            _enemy.setTexture(*curr_bctx().enemy_state()._enemy_texture);
            _enemy.setScale(vec2f(0.5f, 0.5f));
            ssvs::setOrigin(_enemy, ssvs::getLocalCenter);
            _esprite_pos = vec2f(game_constants::width / 2.f,
                game_constants::height / 2.f - 75.f);
        }

        void init_cs_text()
        {
            _scripted_events.reserve(10);

            _t_cs.eff<BTR::Tracking>(-3)
                .eff(sfc::White)
                .in(" ")
                .eff(_ptr_t_cs_wave)
                .in(_ptr_t_cs)
                .mk("");

            _t_cs.setScale(vec2f{3.f, 3.f});

            VRM_CORE_ASSERT(_ptr_t_cs_wave != nullptr);
            VRM_CORE_ASSERT(_ptr_t_cs != nullptr);
        }

        template <typename TFU, typename TFD>
        void add_scripted_event(float time, TFU&& fu, TFD&& fd)
        {
            scripted_event se;
            se._time = ssvu::getSecondsToFT(time);
            se._f_update = fu;
            se._f_draw = fd;

            _scripted_events.emplace_back(std::move(se));
        }

        void add_scripted_text(float time, const std::string& s)
        {
            bool plyd{false};
            add_scripted_event(time,
                [this, time, s, plyd](float t, ft dt) mutable
                {
                    if(!plyd)
                    {
                        assets().psnd(assets().scripted_text);
                        plyd = true;
                    }

                    ssvs::setOrigin(_t_cs, ssvs::getLocalCenter);
                    _t_cs.setAlign(ssvs::TextAlign::Center);
                    _t_cs.setPosition(game_constants::width / 2.f,
                        game_constants::height / 2.f);

                    VRM_CORE_ASSERT(_ptr_t_cs_wave != nullptr);
                    if(t >=
                        ssvu::getSecondsToFT(time) - ssvu::getSecondsToFT(1.f))
                    {
                        _ptr_t_cs_wave->amplitude = std::max(
                            0.f, (t - ssvu::getSecondsToFT(1.f)) * 1.5f);
                    }
                    else
                    {
                        _ptr_t_cs_wave->amplitude = 0;
                    }


                    VRM_CORE_ASSERT(_ptr_t_cs != nullptr);
                    _ptr_t_cs->setStr(s);
                    _t_cs.update(dt);
                },
                [this]
                {
                    this->app().render(_t_cs);
                });
        }

        void game_over() { _state = battle_screen_state::to_game_over; }
        void success() { _state = battle_screen_state::to_next_ctx; }

        void start_enemy_turn()
        {
            _state = battle_screen_state::before_enemy_turn;
        }

        void end_enemy_turn()
        {
            _state = battle_screen_state::before_player_turn;
        }

        void execute_ritual(ritual_maker rm)
        {
            add_scripted_text(1.7f, rm.label());

            _success_effect = rm.effect();
            _state = battle_screen_state::player_ritual;
            auto time_as_ft(ssvu::getSecondsToFT(rm.time()));
            _ritual_ctx.set_and_start_minigame(time_as_ft, rm.make());
            assets().psnd_one(assets().click0);
        }

        void fill_attack_menu()
        {
            auto& m(*_m_ritual_atk);
            m.clear();

            auto& ps(curr_bctx().player_state());
            auto& pst(curr_bctx().player().stats());

            ps.for_atk_rituals([this, &pst, &m](auto& rr)
                {
                    m.emplace_choice(rr.label(), rr.desc(),
                        [this, &pst, &rr](auto& bm)
                        {
                            if(rr.req_mana() <= pst.mana())
                            {
                                pst.mana() -= rr.req_mana();
                                this->execute_ritual(rr);
                                bm.pop_screen();
                            }
                            else
                            {
                                this->display_msg_box("Not enough mana.");
                            }
                        });
                });

            m.emplace_choice("Go back", "", [](auto& bm)
                {
                    bm.pop_screen();
                });
        }

        void fill_utility_menu()
        {
            auto& m(*_m_ritual_utl);
            m.clear();

            auto& ps(curr_bctx().player_state());
            auto& pst(curr_bctx().player().stats());

            ps.for_utl_rituals([this, &pst, &m](auto& rr)
                {
                    m.emplace_choice(rr.label(), rr.desc(),
                        [this, &pst, &rr](auto& bm)
                        {
                            if(rr.req_mana() <= pst.mana())
                            {
                                pst.mana() -= rr.req_mana();
                                this->execute_ritual(rr);
                                bm.pop_screen();
                            }
                            else
                            {
                                this->display_msg_box("Not enough mana.");
                            }
                        });
                });

            m.emplace_choice("Go back", "", [](auto& bm)
                {
                    bm.pop_screen();
                });
        }

        void display_msg_box(const std::string& s)
        {
            auto& mbs(app().make_screen<msgbox_screen>());
            auto& btr(mbs.btr());

            assets().psnd(assets().msgbox);

            btr.eff<BTR::Tracking>(-3).eff(sfc::White).in(s);
            app().push_screen(mbs);
        }

        void fill_main_menu()
        {
            auto& m(*_m_main);
            m.clear();

            m.emplace_choice("Attack rituals", "", [this](auto& bm)
                {
                    this->fill_attack_menu();
                    bm.push_screen(*_m_ritual_atk);
                });
            m.emplace_choice("Utility rituals", "", [this](auto& bm)
                {
                    this->fill_utility_menu();
                    bm.push_screen(*_m_ritual_utl);
                });
            m.emplace_choice("Inspect enemy", "", [this](auto&)
                {
                    const auto& es(this->curr_bctx().enemy().stats());

                    std::ostringstream oss;
                    oss << "Inspecting enemy...\n\n";
                    oss << "Health: " << es.health() << " / " << es.maxhealth()
                        << "\n";
                    oss << "Shield: " << es.shield() << " / " << es.maxshield()
                        << "\n";
                    oss << "Power: " << es.power() << "\n";

                    this->display_msg_box(oss.str());
                });

            auto& ps(curr_bctx().player_state());
            ps.for_mana_rituals([this, &m](auto& rr)
                {
                    m.emplace_choice(rr.label(), rr.desc(), [this, &rr](auto&)
                        {
                            this->execute_ritual(rr);
                        });
                });
        }

        void init_menu()
        {
            _m_main = &_menu.make_screen();
            _m_ritual_atk = &_menu.make_screen();
            _m_ritual_utl = &_menu.make_screen();

            fill_main_menu();

            _menu.on_change += [this]
            {
                _menu_gfx_state.rebuild_from(*this, _menu);
            };

            _menu.push_screen(*_m_main);
            update_menu(1.f);
        }

        void ritual_failure()
        {
            app()._shake = 40;
            add_scripted_text(1.1f, "Failure!");
            assets().psnd_one(assets().failure);
        }
        void ritual_success()
        {
            assets().psnd_one(assets().success);
            add_scripted_text(1.1f, "Success!");

            _success_effect(curr_bctx());
        }


        void update_stat_bars()
        {
            _player_stats_gfx.refresh(curr_bctx().player().stats());
            _enemy_stats_gfx.refresh(curr_bctx().enemy().stats());
        }


        void update_menu(ft dt) { _menu_gfx_state.update(app(), _menu, dt); }
        void draw_menu()
        {
            app().render(_bar);
            _menu_gfx_state.draw(app().window());
        }

        void draw_stats_bars()
        {
            app().render(_player_stats_gfx);
            app().render(_enemy_stats_gfx);
        }

        void update_ritual(ft dt)
        {
            _ritual_ctx.update(dt);

            if(_ritual_ctx.is_in_progress())
            {
            }
            else if(_ritual_ctx.is_failure())
            {
                ritual_failure();
                start_enemy_turn();
            }
            else if(_ritual_ctx.is_success())
            {
                ritual_success();
                start_enemy_turn();
            }
        }
        void draw_ritual() { _ritual_ctx.draw(); }

        void update_enemy(ft) {}
        void draw_enemy() {}

        void update_scripted_events(ft dt)
        {
            VRM_CORE_ASSERT(!_scripted_events.empty());

            auto& curr(_scripted_events.front());
            curr.update(dt);

            ssvu::eraseRemoveIf(_scripted_events, [](const auto& x)
                {
                    return x._time <= 0.f;
                });
        }
        void draw_scripted_events()
        {
            VRM_CORE_ASSERT(!_scripted_events.empty());
            auto& curr(_scripted_events.front());
            curr._f_draw();
        }

        auto m_player_name() const { return std::string{"The player "}; }
        auto m_enemy_name() const { return std::string{"The enemy "}; }

        auto m_health(stat_value x) const
        {
            return std::string{" "} + std::to_string((int)x) +
                   std::string{" health points"};
        }

        auto m_shield(stat_value x) const
        {
            return std::string{" "} + std::to_string((int)x) +
                   std::string{" shield points"};
        }

    private:
        std::vector<std::string> _next_notifications;

    public:
        void append_turn_notification(const std::string& s)
        {
            _next_notifications.emplace_back(s + ".");
        }

        void event_listener(battle_event be)
        {
            auto apnd_dmg = [this](auto&& name_fn, auto&& my_be)
            {
                append_turn_notification(
                    name_fn() + "was damaged for\n" +
                    m_health((int)(my_be.e_damage()._amount)));
            };

            auto apnd_sdmg = [this](auto&& name_fn, auto&& my_be)
            {
                append_turn_notification(
                    name_fn() + "shield was damaged for\n" +
                    m_shield((int)(my_be.e_damage()._amount)));
            };

            auto apnd_hl = [this](auto&& name_fn, auto&& my_be)
            {
                append_turn_notification(
                    name_fn() + "was healed for\n" +
                    m_health((int)(my_be.e_heal()._amount)));
            };

            auto apnd_shl = [this](auto&& name_fn, auto&& my_be)
            {
                append_turn_notification(
                    name_fn() + "shield was restored for\n" +
                    m_shield((int)(my_be.e_heal()._amount)));
            };

            auto apnd_stn = [this](auto&& name_fn, auto&& my_be)
            {
                append_turn_notification(
                    name_fn() + "was stunned for for\n" +
                    m_shield((int)(my_be.e_stun()._turns)) + " turns");
            };

            switch(be.type())
            {
                case battle_event_type::enemy_damaged:
                    apnd_dmg(
                        [this]
                        {
                            return m_enemy_name();
                        },
                        be);
                    _enemy_shake = be.e_damage()._amount * 3;
                    break;

                case battle_event_type::player_damaged:
                    apnd_dmg(
                        [this]
                        {
                            return m_player_name();
                        },
                        be);
                    assets().psnd_one(assets().enemy_atk0, assets().enemy_atk1);
                    break;

                case battle_event_type::enemy_shield_damaged:
                    apnd_sdmg(
                        [this]
                        {
                            return m_enemy_name();
                        },
                        be);

                    _enemy_shake = be.e_damage()._amount * 2;
                    break;

                case battle_event_type::player_shield_damaged:
                    apnd_sdmg(
                        [this]
                        {
                            return m_player_name();
                        },
                        be);

                    assets().psnd_one(assets().enemy_atk0, assets().enemy_atk1);
                    break;

                case battle_event_type::enemy_healed:
                    apnd_hl(
                        [this]
                        {
                            return m_enemy_name();
                        },
                        be);

                    assets().psnd_one(assets().shield_up);
                    break;

                case battle_event_type::player_healed:
                    apnd_hl(
                        [this]
                        {
                            return m_player_name();
                        },
                        be);

                    assets().psnd_one(assets().shield_up);
                    break;

                case battle_event_type::enemy_shield_healed:
                    apnd_shl(
                        [this]
                        {
                            return m_enemy_name();
                        },
                        be);

                    assets().psnd_one(assets().shield_up);
                    break;

                case battle_event_type::player_shield_healed:
                    apnd_shl(
                        [this]
                        {
                            return m_player_name();
                        },
                        be);

                    assets().psnd_one(assets().shield_up);
                    break;

                case battle_event_type::enemy_stunned:
                    apnd_stn(
                        [this]
                        {
                            return m_enemy_name();
                        },
                        be);
                    break;
            }
        }

        void init_battle()
        {
            auto& battle(curr_bctx().battle());
            curr_bctx().on_event += [this](auto e)
            {
                this->event_listener(e);
            };

            // Assume player starts
            if(battle.is_player_turn())
            {
                _state = battle_screen_state::player_menu;
            }
            else
            {
            }
        }

    public:
        battle_screen(game_app& app,
            std::vector<std::unique_ptr<battle_context_t>>&& ctxs) noexcept
            : base_type(app),
              _ctxs(std::move(ctxs)),
              _ritual_ctx{app}
        {
            init_menu_bg();
            init_cs_text();
            init_menu();
            init_battle();

            add_scripted_text(1.7f, "Battle start!");

            app.setup_music(assets().music);
        }

        void update(ft dt) override
        {
            update_stat_bars();
            auto f_off(vec2f{0, std::sin(_enemy_f) * _enemy_f_magnitude});

            if(_enemy_shake > 0)
            {
                _enemy_shake -= dt;
                auto s(std::abs(_enemy_shake));
                vec2f offset(
                    ssvu::getRndR(-s, s + 0.1f), ssvu::getRndR(-s, s + 0.1f));
                _enemy.setPosition(_esprite_pos + f_off + offset);

                return;
            }
            else
            {
                _enemy_f += dt * 0.06f;
                _enemy_shake = 0;
                _enemy.setPosition(_esprite_pos + f_off);
            }

            if(!_scripted_events.empty())
            {
                update_scripted_events(dt);
                return;
            }

            if(!_next_notifications.empty())
            {
                std::string acc;
                for(const auto& nn : _next_notifications) acc += nn + "\n";
                display_msg_box(acc);

                _next_notifications.clear();
            }

            if(_state == battle_screen_state::player_menu)
            {
                update_menu(dt);
            }
            else if(_state == battle_screen_state::player_ritual)
            {
                update_ritual(dt);
            }
            else if(_state == battle_screen_state::enemy_turn)
            {
                // update_enemy(dt);
                curr_bctx().enemy_state()._f_ai(*this, curr_bctx());
            }
            else if(_state == battle_screen_state::before_enemy_turn)
            {
                // TODO: check for enemy death here
                if(curr_bctx().enemy().stats().health() <= 0)
                {
                    success();
                }
                else
                {
                    add_scripted_text(1.7f, "Enemy turn!");
                    _state = battle_screen_state::enemy_turn;
                }
            }
            else if(_state == battle_screen_state::before_player_turn)
            {

                if(curr_bctx().player().stats().health() <= 0 &&
                    _state != battle_screen_state::to_game_over)
                {
                    game_over();
                }
                else
                {
                    add_scripted_text(1.7f, "Player turn!");
                    _state = battle_screen_state::player_menu;
                }
            }
            else if(_state == battle_screen_state::to_next_ctx)
            {
                ++_ctx_idx;
                //_enemy.setTexture(*curr_bctx().enemy_state()._enemy_texture);
                _enemy = sf::Sprite{*curr_bctx().enemy_state()._enemy_texture};
                _enemy.setScale(vec2f(0.5f, 0.5f));
                ssvs::setOrigin(_enemy, ssvs::getLocalCenter);

                if(_ctx_idx < 4)
                {
                    display_msg_box("The next demon approaches...");
                    _state = battle_screen_state::player_menu;
                }
                else
                {
                    display_msg_box("You won!");
                }
            }
            else if(_state == battle_screen_state::to_game_over)
            {
                _state = battle_screen_state::game_over;
                display_msg_box("Game over!");
            }
            else if(_state == battle_screen_state::game_over)
            {
                app().pop_screen();
            }
        }

        void draw() override
        {
            app().render(_landscape);
            app().render(_enemy);


            if(!_scripted_events.empty())
            {
                draw_scripted_events();
                return;
            }



            if(_state == battle_screen_state::player_menu)
            {
                draw_menu();
                draw_stats_bars();
            }
            else if(_state == battle_screen_state::player_ritual)
            {
                draw_ritual();
            }
            else if(_state == battle_screen_state::enemy_turn)
            {
                draw_enemy();
                draw_stats_bars();
            }
        }
    };

    void battle_menu_gfx_button::update(game_app & app, battle_menu & bm, ft dt)
    {
        _shape.setPosition(_pos);
        sf::Color cuh{0, 0, 0, 230};
        sf::Color ch{0, 0, 0, 130};

        _shape.setOutlineColor(sfc::Black);
        _shape.setOutlineThickness(3);
        _shape.setFillColor(is_hovered(app) ? ch : cuh);

        auto lbtn_down(app.lb_down());
        auto rbtn_down(app.rb_down());

        if(is_hovered(app) && !bm._was_pressed)
        {
            if(lbtn_down)
            {
                assets().psnd(assets().blip);
                bm._was_pressed = true;
                _bmc.execute(bm);
            }
            else if(rbtn_down)
            {
                const auto& d(_bmc.desc());
                if(d != "")
                {
                    assets().psnd(assets().blip);
                    bm._was_pressed = true;
                    _bs.display_msg_box(d);
                }
            }
        }

        if(!lbtn_down && !rbtn_down)
        {
            bm._was_pressed = false;
        }

        _tr.setPosition(_pos + vec2f(5.f, 5.f));

        ssvs::setOrigin(_tr, ssvs::getLocalCenter);
        _tr.setPosition(ssvs::getGlobalCenter(_shape));
        _tr.update(dt);
    }

    struct title_screen : public game_screen
    {
        float _safety{50};

        ssvs::BitmapTextRich _t_cs{*assets().fontObBig};
        ssvs::BitmapTextRich _t_cs2{*assets().fontObBig};

        sf::Sprite _bg{*assets().title};

        std::function<void()> on_start;
        title_screen(game_app& app) : game_screen(app)
        {
            _bg.setPosition(0.f, 0.f);


            _t_cs.eff<BTR::Tracking>(-3)
                .eff(sfc::Red)
                .eff<BTR::Wave>(1.5f, 0.03f)
                .in("Demon Cleansing");

            _t_cs2.eff<BTR::Tracking>(-3)
                .eff(sfc::White)
                .eff<BTR::Wave>(1.0f, 0.02f)
                .in("Press LMB to play.");

            _t_cs.setScale(vec2f(3.f, 3.f));
            _t_cs2.setScale(vec2f(2.f, 2.f));
        }

        void update(ft dt) override
        {
            if(_safety >= 0.f)
            {
                _safety -= dt;
            }
            else if(app().lb_down())
            {
                on_start();
                _safety = 50.f;
            }

            ssvs::setOrigin(_t_cs, ssvs::getLocalCenter);
            ssvs::setOrigin(_t_cs2, ssvs::getLocalCenter);

            _t_cs.setPosition(game_constants::width / 2.f,
                game_constants::height / 2.f - 10.f);
            _t_cs.update(dt);

            _t_cs2.setPosition(game_constants::width / 2.f,
                game_constants::height / 2.f + 200.f);

            _t_cs2.update(dt);
        }

        void draw() override
        {
            app().render(_bg);
            // app().render(_t_cs);
            app().render(_t_cs2);
        }
    };
}
GGJ16_NAMESPACE_END

void fill_ps(ggj16::cplayer_state& ps)
{
    using namespace ggj16;

    ps.emplace_atk_ritual<symbol_ritual>("Fireball",
        "Easy ritual.\nConnect the dots.\nLow HP damage.\nMinimal shield "
        "damage.",
        ritual_type::complete, 4, 15,
        [](symbol_ritual& sr)
        {
            sr.add_point({{-30.f * 2.8f, 60.f * 2.8f}, 10.f});
            sr.add_point({{0.f * 2.8f, -60.f * 2.8f}, 10.f});
            sr.add_point({{30.f * 2.8f, 60.f * 2.8f}, 10.f});
            sr.add_point({{-50.f * 2.8f, -10.f * 2.8f}, 10.f});
            sr.add_point({{50.f * 2.8f, -10.f * 2.8f}, 10.f});
        },
        [](battle_context_t& c)
        {
            assets().psnd_one(assets().fireball);

            c.damage_enemy_by(20);
            c.damage_enemy_shield_by(5);
        });

    ps.emplace_atk_ritual<aura_ritual>("Rend shield",
        "Medium ritual.\nPrevent dots from disappearing.\nMinimal HP "
        "damage.\nLarge shield damage.",
        ritual_type::resist, 6, 20,
        [](aura_ritual& sr)
        {
            sr.add_point({{-0.f, -45.f * 3.5f}, 20.f});
            sr.add_point({{-31.f * 2.8f, 31.f * 3.5f}, 20.f});
            sr.add_point({{31.f * 2.8f, 31.f * 3.5f}, 20.f});
        },
        [](battle_context_t& c)
        {
            assets().psnd_one(assets().fireball);
            c.damage_enemy_by(5);
            c.damage_enemy_shield_by(25);
        });

    ps.emplace_atk_ritual<symbol_ritual>("Obliterate",
        "Hard ritual.\nConnect the dots.\nMassive HP damage.\nLow shield "
        "damage.",
        ritual_type::complete, 4, 50,
        [](symbol_ritual& sr)
        {
            auto x(-1024 / 2.f);
            auto y(-768 / 2.f);
            int x_count = 6;
            int y_count = 3;
            auto offset = 40.f;

            auto fullx = game_constants::width - offset * 2;
            auto fully = game_constants::height - offset * 2;

            auto xinc(fullx / x_count);
            auto yinc(fully / y_count);


            for(int ix = 0; ix < x_count; ++ix)
            {
                sr.add_point({{x + offset + xinc * ix, y + offset}, 12.f});
            }

            for(int iy = 0; iy < y_count + 1; ++iy)
            {
                sr.add_point(
                    {{x + offset + fullx, y + offset + yinc * iy}, 12.f});
            }


            for(int ix = x_count - 1; ix >= 0; --ix)
            {
                sr.add_point({{x + offset + xinc * ix,
                                  y + game_constants::height - offset},
                    12.f});
            }

            for(int iy = y_count - 1; iy > 0; --iy)
            {
                sr.add_point({{x + offset, y + offset + yinc * iy}, 12.f});
            }
        },
        [](battle_context_t& c)
        {
            assets().psnd_one(assets().obliterate);
            c.damage_enemy_shield_by(20);
            c.damage_enemy_by(60);
        });

    ps.emplace_utl_ritual<drag_ritual>("Heal",
        "Easy ritual.\nCollect the dots.\nMedium HP heal.\nMinimal shield "
        "self-damage.",
        ritual_type::complete, 5, 30,
        [](drag_ritual& sr)
        {
            sr.add_target(
                {game_constants::width / 2.f, game_constants::height / 2.f});

            for(int i = 0; i < 6; ++i)
            {
                auto offset(20.f * 2.8f);
                auto x(ssvu::getRndR(offset, game_constants::width - offset));
                auto y(ssvu::getRndR(offset, game_constants::height - offset));
                sr.add_draggable(vec2f{x, y});
            }
        },
        [](battle_context_t& c)
        {
            c.damage_player_shield_by(10);
            c.heal_player_by(35);
        });

    ps.emplace_utl_ritual<drag_ritual>("Repair shield",
        "Medium ritual.\nCollect the dots.\nMinimal HP "
        "self-damage.\nMedium "
        "shield restoration.",
        ritual_type::complete, 6, 40,
        [](drag_ritual& sr)
        {
            sr.add_target(
                {game_constants::width / 2.f, game_constants::height / 2.f});

            for(int i = 0; i < 3; ++i)
            {
                auto offset(20.f * 2.8f);
                sr.add_draggable(vec2f{offset, offset + (i * 60)});
                sr.add_draggable(
                    vec2f{game_constants::width - offset, offset + (i * 60)});
            }
        },
        [](battle_context_t& c)
        {
            c.damage_player_by(5);
            c.heal_player_shield_by(25);
        });


    ps.emplace_mana_ritual<aura_ritual>("Restore mana",
        "Medium ritual.\nRestores your mana.", ritual_type::resist, 4, 0,
        [](aura_ritual& sr)
        {
            auto offset(30.f);
            //  vec2f center(
            //       game_constants::width / 2.f, game_constants::height /
            //       2.f);

            for(int i = -3; i < 4; ++i)
            {
                sr.add_point({vec2f{offset * i, offset * i}, 20.f});
            }

        },
        [](battle_context_t& c)
        {
            assets().psnd_one(assets().shield_up);
            c.restore_player_mana();
        });
}

auto first_ai()
{
    using namespace ggj16;

    return [](battle_screen& bs, battle_context_t& bc)
    {
        auto& ps(bc.player().stats());
        auto& es(bc.enemy().stats());

        if(es.health() <= es.maxhealth() * 0.2)
        {
            bs.display_msg_box("The demon attempts to heal himself!");
            bc.heal_enemy_by(10);
            bc.damage_enemy_shield_by(5);
        }
        else
        {
            bs.display_msg_box("The demon pounces at the player.");
            bc.damage_player_by(30);
            bc.damage_player_shield_by(3);
        }

        bs.end_enemy_turn();
    };
}

auto second_ai()
{
    using namespace ggj16;

    return [](battle_screen& bs, battle_context_t& bc)
    {
        auto& ps(bc.player().stats());
        auto& es(bc.enemy().stats());

        if(es.health() <= es.maxhealth() * 0.3)
        {
            bs.display_msg_box("The demon attempts to heal himself!");
            bc.heal_enemy_by(12);
            bc.damage_enemy_shield_by(4);
        }
        else if(ps.shield() >= ps.maxshield() * 0.8)
        {
            bs.display_msg_box("The demon performs\nan armor-piercing attack!");
            bc.damage_player_shield_by(20);
            bc.damage_player_by(5);
        }
        else
        {
            bs.display_msg_box("The demon pounces at the player.");
            bc.damage_player_by(35);
            bc.damage_player_shield_by(4);
        }

        bs.end_enemy_turn();
    };
}

auto third_ai()
{
    using namespace ggj16;

    return [](battle_screen& bs, battle_context_t& bc)
    {
        auto& ps(bc.player().stats());
        auto& es(bc.enemy().stats());

        if(es.health() <= es.maxhealth() * 0.3)
        {
            bs.display_msg_box("The demon attempts to heal himself!");
            bc.heal_enemy_by(14);
            bc.damage_enemy_shield_by(3);
        }
        else if(es.shield() <= es.maxshield() * 0.2)
        {
            bs.display_msg_box("The demon attempts to restore his shield!");
            bc.heal_enemy_shield_by(10);
            bc.damage_enemy_by(5);
        }
        else if(ps.shield() >= ps.maxshield() * 0.7)
        {
            bs.display_msg_box("The demon performs\nan armor-piercing attack!");
            bc.damage_player_shield_by(25);
            bc.damage_player_by(7);
        }
        else
        {
            bs.display_msg_box("The demon pounces at the player.");
            bc.damage_player_by(40);
            bc.damage_player_shield_by(5);
        }

        bs.end_enemy_turn();
    };
}

auto fourth_ai()
{
    using namespace ggj16;

    return [](battle_screen& bs, battle_context_t& bc)
    {
        auto& ps(bc.player().stats());
        auto& es(bc.enemy().stats());

        if(es.health() <= es.maxhealth() * 0.3)
        {
            bs.display_msg_box("The demon attempts to heal himself!");
            bc.heal_enemy_by(18);
            bc.damage_enemy_shield_by(4);
        }
        else if(es.shield() <= es.maxshield() * 0.3)
        {
            bs.display_msg_box("The demon attempts to restore his shield!");
            bc.heal_enemy_shield_by(20);
            bc.damage_enemy_by(5);
        }
        else if(ps.shield() >= ps.maxshield() * 0.6)
        {
            bs.display_msg_box("The demon performs\nan armor-piercing attack!");
            bc.damage_player_shield_by(30);
            bc.damage_player_by(15);
        }
        else
        {
            bs.display_msg_box("The demon pounces at the player.");
            bc.damage_player_by(50);
            bc.damage_player_shield_by(7);
        }

        bs.end_enemy_turn();
    };
}

int main()
{
    using namespace ggj16;

    using game_app_runner = boilerplate::app_runner<game_app>;
    game_app_runner game{
        "Demon Cleansing", game_constants::width, game_constants::height};
    game_app& app(game.app());

    character_stats cs_demon0;
    cs_demon0.health() = cs_demon0.maxhealth() = 50;
    cs_demon0.shield() = cs_demon0.maxshield() = 30;
    cs_demon0.mana() = cs_demon0.maxmana() = 100;
    cs_demon0.power() = 10;

    character_stats cs_demon1;
    cs_demon1.health() = cs_demon1.maxhealth() = 60;
    cs_demon1.shield() = cs_demon1.maxshield() = 35;
    cs_demon1.mana() = cs_demon1.maxmana() = 200;
    cs_demon1.power() = 20;

    character_stats cs_demon2;
    cs_demon2.health() = cs_demon2.maxhealth() = 70;
    cs_demon2.shield() = cs_demon2.maxshield() = 50;
    cs_demon2.mana() = cs_demon2.maxmana() = 300;
    cs_demon2.power() = 30;

    character_stats cs_demon3;
    cs_demon3.health() = cs_demon3.maxhealth() = 100;
    cs_demon3.shield() = cs_demon3.maxshield() = 60;
    cs_demon3.mana() = cs_demon3.maxmana() = 400;
    cs_demon3.power() = 40;

    character_stats cs_player;
    cs_player.health() = cs_player.maxhealth() = 100;
    cs_player.shield() = cs_player.maxshield() = 50;
    cs_player.mana() = cs_player.maxmana() = 100;
    cs_player.power() = 100;

    battle_participant demon0{cs_demon0};
    cenemy_state es_d0{assets().d0};
    es_d0._f_ai = first_ai();

    battle_participant demon1{cs_demon1};
    cenemy_state es_d1{assets().d1};
    es_d1._f_ai = second_ai();

    battle_participant demon2{cs_demon2};
    cenemy_state es_d2{assets().d2};
    es_d2._f_ai = third_ai();

    battle_participant demon3{cs_demon3};
    cenemy_state es_d3{assets().d3};
    es_d3._f_ai = fourth_ai();

    battle_participant bplayer{cs_player};
    cplayer_state ps;
    fill_ps(ps);

    battle_t b0{bplayer, demon0};
    battle_t b1{bplayer, demon1};
    battle_t b2{bplayer, demon2};
    battle_t b3{bplayer, demon3};

    auto battle_set = [=, &app, &ps]() mutable
    {
        auto bc0(std::make_unique<battle_context_t>(ps, es_d0, b0));
        auto bc1(std::make_unique<battle_context_t>(ps, es_d1, b1));
        auto bc2(std::make_unique<battle_context_t>(ps, es_d2, b2));
        auto bc3(std::make_unique<battle_context_t>(ps, es_d3, b3));

        std::vector<std::unique_ptr<battle_context_t>> bcs;
        bcs.emplace_back(std::move(bc0));
        bcs.emplace_back(std::move(bc1));
        bcs.emplace_back(std::move(bc2));
        bcs.emplace_back(std::move(bc3));

        auto& s_battle(app.make_screen<battle_screen>(std::move(bcs)));

        s_battle.reset();
        app.push_screen(s_battle);
    };

    auto& s_title(app.make_screen<title_screen>());
    s_title.on_start = [battle_set]() mutable
    {
        battle_set();
    };
    app.push_screen(s_title);

    game.run();
    return 0;
}
